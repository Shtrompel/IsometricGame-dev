#ifndef GAME_QUAD_TREE
#define GAME_QUAD_TREE

#ifdef QUAD_TREE_PRINT
#include <iostream>
#endif

#include <algorithm>
#include <cassert>
#include <type_traits> // std::is_signed
#include <vector>
#include <stack>
#include <deque>
#include <limits>  // std::numeric_limits
#include <numeric> // std::accumulate

#include "utils/math.hpp"

#include <SFML/System/Vector2.hpp>

/**
    
    QUAD_TREE_PRINT
    QUAD_TREE_INFO
    QUAD_TREE_DEBUG
*/

template <typename T>
using t_vec = sf::Vector2<T>;

// Expanding in size and dynamic
template <typename Type, typename NumType = int32_t>
struct ChunkQuadTree
{
	static_assert(std::is_signed<NumType>::value);

	struct Node;
	struct Point;

	// Can be replaced by an integer
	// Float in case of "Type" data will be used with
	// floating type instead of "Point::pos"
	typedef NumType t_float;
	typedef t_vec<t_float> FVec;

	typedef t_vec<NumType> Vec;
	typedef typename std::vector<Point>::iterator t_point_itr;
	typedef std::pair<Node *, t_point_itr> t_pair;

	constexpr static size_t INFINITE_LOOP = 8;
	constexpr static size_t MAX_SIZE = std::numeric_limits<size_t>::max();
	constexpr static NumType MAX_NUM = std::numeric_limits<NumType>::max();
	constexpr static NumType MAX_FLOAT = std::numeric_limits<t_float>::max();

	static inline auto DEFAULT_CONDITION = [](const Vec &, const Type &, const NumType &) { return true; };
	typedef decltype(DEFAULT_CONDITION) t_def_condition;

	static inline auto DEFAULT_COMPARATOR =
		[](
			const Vec &center,
			const Vec &a,
			const Type &,
			const Vec &b,
			const Type &) {
			return vec_distsq(center, a) < vec_distsq(center, b);
		};
	typedef decltype(DEFAULT_COMPARATOR)
		t_def_comparator;

	Type defaultType = Type();
	Vec minSize;
	size_t maxPoints = 4;
	size_t percision = 0;

	Node *root = nullptr;
	bool hardopt = false;

	template <class U>
	struct DistPair
	{
		bool operator<(const DistPair<U> &other) const
		{
			return this->dist < other.dist;
		}
		t_float dist;
		U data;
	};

	struct Point
	{
		Vec pos;
		Type type;

		Point(Vec pos, Type type)
			: pos(pos), type(type)
		{
		}

		bool operator==(const Point &other) const
		{
			return this->type == other.type && this->pos == other.pos;
		}

		bool operator==(const Type &type) const
		{
			return this->type == type;
		}

		bool operator==(const Vec &vec) const
		{
			return this->pos == vec;
		}
	};

	struct Node
	{
		std::vector<Point> points;
		Node *next[4] = {nullptr};
		const size_t count = 4, cx = 2, cy = 2;
		Node *parent = nullptr;
		Vec pos, size = {(NumType)0, (NumType)0};

		bool _checked = false;

		Node(Vec pos)
			: pos(pos)
		{
		}

		Node(const Vec &pos, const Vec &size, Node *parent = nullptr)
			: parent(parent), pos(pos), size(size)
		{
		}

		inline void add(const Point &point)
		{
			points.push_back(point);
		}

		inline bool is_leaf() const
		{
			for (size_t i = 0u; i < 4u; ++i)
				if (next[i])
					return false;
			return true;
		}

		inline bool empty() const
		{
			return points.empty() && is_leaf();
		}

		inline bool inside(const Vec &in) const
		{
			if (size.x == (NumType)0 && size.y == (NumType)0)
				return in == pos;
			return vec_inside(in, pos, size);
		}

		inline bool is_point() const
		{
			return size.x == (NumType)0 && (NumType)size.y == 0;
		}

		inline NumType distsq(const Vec &v) const
		{
			return (v.x * pos.x) * (v.x * pos.x) +
				   (v.y * pos.y) * (v.y * pos.y);
		}

		inline Node *get(const Vec &vec) const
		{
			Vec delta = vec - this->pos;
			size_t x = (size_t)(delta.x * cx / size.x);
			size_t y = (size_t)(delta.y * cy / size.y);
			size_t index = x + y * cx;
			return ((index < count) ? next[index] : nullptr);
		}

		inline Node *create(const Vec &vec)
		{
			Vec delta = vec - this->pos;
			Vec nSize = {this->size.x / (NumType)cx, this->size.y / (NumType)cy};
			size_t x = (size_t)(delta.x / nSize.x);
			size_t y = (size_t)(delta.y / nSize.y);

			size_t index = (size_t)(x + y * cx);

			if (index < cx * cy)
			{
				Vec pos = this->pos;
				pos.x += nSize.x * (NumType)x;
				pos.y += nSize.y * (NumType)y;
				return (this->next[index] = new Node{pos, nSize, this});
			}
			else
			{
#ifdef QUAD_TREE_INFO
				printf("%d %d - %d\n", x, y, cx * cy);
				assert(NULL);
#endif // QUAD_TREE_INFO
				return nullptr;
			}
		}
	};

	struct Iterator
	{
		using iterator_category = std::forward_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using value_type = Type;
		using pointer = value_type *;
		using reference = value_type &;

		void next()
		{
			if (stack.empty())
				return;
			assert(stack.top());
			Node *top = stack.top();
			stack.pop();
			for (size_t i = 0u; i < 4u; ++i)
			{
				if (top->next[i])
				{
#ifdef QUAD_TREE_INFO
					printNode("", top->next[i]);
#endif // QUAD_TREE_INFO
					stack.push(top->next[i]);
				}
			}
		}

		Iterator(Node *node) : node(node)
		{
			if (!node)
				return;
			stack.push(node);
			while (stack.size() && stack.top()->points.empty())
				next();
			if (stack.size())
				node = stack.top();
		}

		inline Vec &pos()
		{
			return stack.top()->points[pointIndex].pos;
		}

		reference operator*() const
		{
			assert(stack.top());
			assert(!stack.top()->points.empty());
			assert(pointIndex < stack.top()->points.size());
			return stack.top()->points[pointIndex].type;
		}

		pointer operator->()
		{
			assert(stack.top());
			assert(!stack.top()->points.empty());
			assert(pointIndex < stack.top()->points.size());
			return &stack.top()->points[pointIndex].type;
		}

		Iterator &operator++(void)
		{
			if (stack.size())
			{
				Node *top = stack.top();
				pointIndex++;
				if (pointIndex >= top->points.size())
				{
					pointIndex = 0u;
					next();
					while (stack.size() && stack.top()->points.empty())
					{
						next();
					}
				}
				if (stack.size())
					node = stack.top();
			}
			return *this;
		}

		Iterator operator++(int)
		{
			Iterator tmp = *this;
			++*tmp;
			return tmp;
		}

		friend bool operator==(const Iterator &a, const Iterator &b)
		{
			if (a.stack.empty() || b.stack.empty())
				return a.stack.empty() && b.stack.empty();
			return a.stack.top() == b.stack.top();
		}

		friend inline bool operator!=(const Iterator &a, const Iterator &b)
		{
			return !(a == b);
		};

		std::stack<Node *> stack;
		Node *node = nullptr;
		size_t pointIndex = 0u;
	};

	static void printQuadTree(Node *node, int level)
	{
#ifdef QUAD_TREE_PRINT
		if (!node)
			return;
		std::cout << "Lvl: " << level << ", ";
		std::cout << "Pos: " << node->pos.x << ", " << node->pos.y;
		std::cout << " ";
		std::cout << "Size " << node->size.x << ", " << node->size.y;
		std::cout << "\n";

		for (size_t i = 0; i < node->points.size(); ++i)
		{
			std::cout << i << " Particle "
					  << " : ";
			auto p = node->points.at(i);
			std::cout << "Pos: " << p.pos.x << ", " << p.pos.y << "\n";
		}

		for (size_t i = 0u; i < 4u; ++i)
			printQuadTree(node->next[i], level + 1);
#endif
	}

	static void printQuadTree(Node *node)
	{
#ifdef QUAD_TREE_PRINT
		printQuadTree(node, 0);
#endif
	}

	static void printNode(const char *sex, Node *node)
	{
#ifdef QUAD_TREE_PRINT
		assert(node);
		std::cout << sex << " " << (void *)node;
		std::cout << ": ";
		std::cout << "Pos: " << node->pos.x << ", " << node->pos.y;
		std::cout << ", ";
		std::cout << "Size " << node->size.x << ", " << node->size.y;
		std::cout << '\n';
#endif
	}

	static void printPtrPoint(const char *sex, const Point &point)
	{
#ifdef QUAD_TREE_PRINT
		std::cout << sex << " ";
		std::cout << (void *)(point.type);
		std::cout << ": ";
		std::cout << "Pos: " << point.pos.x << ", " << point.pos.y;
		std::cout << '\n';
#endif
	}

	static void printPtrType(const char *sex, const Type &type)
	{
#ifdef QUAD_TREE_PRINT
		std::cout << sex << std::hex << reinterpret_cast<size_t>(type);
		std::cout << "\n";
#endif
	}

	ChunkQuadTree(const Vec &limit = Vec{1, 1}, const size_t maxPoints = 4)
		: minSize(limit), maxPoints(maxPoints)
	{
	}

	void clear()
	{
		destroy(this->root);
	}

	template <typename NT2>
	static bool compare_trees(
		Node *a,
		typename ChunkQuadTree<Type, NT2>::Node *b,
		int level = 0)
	{
		if (a && b)
		{
			if (!std::equal(
					a->points.begin(),
					a->points.end(),
					b->points.begin(),
					[](const Point &pa, const typename ChunkQuadTree<Type, NT2>::Point &pb) {
						return pa.type == pb.type && (FVec)pa.pos == (FVec)pb.pos;
					}))
			{
				return false;
			}

			bool fine = true;
			for (size_t i = 0u; i < 4u; ++i)
				fine &= compare_trees<NT2>(a->next[i], b->next[i], level + 1);
			return fine;
		}

		return a == b;
	}

	template <typename NT2 = NumType>
	inline bool operator==(typename ChunkQuadTree<Type, NT2>::Node &other)
	{
		return compare_trees(this->root, other.root);
	}

	t_pair full_find(const Point &u)
	{
		int found = 0;
		std::stack<Node *> s;
		s.push(root);
		t_pair ret = {nullptr, t_point_itr{}};
		while (s.size())
		{
			Node *top = s.top();
			s.pop();
			for (int i = 0; i < 4; ++i)
				if (top->next[i])
					s.push(top->next[i]);
			for (auto itr = top->points.begin();
				 itr != top->points.end();
				 ++itr)
			{
				if (!top->inside(itr->pos))
				{
#ifdef QUAD_TREE_INFO
					printPtrPoint("Point", *itr);
					printNode("Outside of", top);
#endif // QUAD_TREE_INFO
					assert(0);
				}
				if (*itr == u && top->is_leaf())
				{
#ifdef QUAD_TREE_INFO
					if (found)
					{
						printf("Two instances of type:\n");
						printPtrPoint("First", *(ret.second));
						printPtrPoint("Second", *(itr));
					}
#endif // QUAD_TREE_INFO
					ret = {top, itr};
					found++;
				}
			}
		}
		assert(found);
		assert(found <= 1);

		return ret;
	}

	inline Type &at(const Vec &vec)
	{
		t_pair pair = get_pair(vec);
		return pair.first ? pair.second->type : defaultType;
	}

	inline Type &at(const Vec &vec, const Type &type)
	{
		t_pair pair = get_pair(vec, type);
		return pair.first ? pair.second->type : defaultType;
	}

	inline bool insert(const Vec &vec, const Type &type = Type())
	{
		Point p{vec, type};
		return insert(p);
	}

	bool insert(const Point &p)
	{
		bool isRoot = root;

		// If there's is no root node, create a node on the
		// point with zero size
		if (!isRoot)
		{
			root = new Node{p.pos};
			root->add(p);
			return true;
		}

		// If there's only node, expand it
		if (root->is_point())
		{
			Vec pos, size;
			auto sizeCalc = [](NumType x) {
				NumType ret = 1 << (NumType)(log_2(x) + 2);
				//assert(x <= ret);
				return ret;
			};
			/*
            auto posFloor = [](NumType x, NumType s) {
                return math_floor(x / s) * s;
            };
            pos.x = posFloor(math_min(root->pos.x, p.pos.x), 2);
            pos.y = posFloor(math_min(root->pos.y, p.pos.y), 2);
            */

			pos.x = math_min(root->pos.x, p.pos.x);
			pos.y = math_min(root->pos.y, p.pos.y);

			NumType width = sizeCalc(math_abs(root->pos.x - p.pos.x));
			NumType height = sizeCalc(math_abs(root->pos.y - p.pos.y));
			width = math_max(width, height);
			size.x = width;
			size.y = width;
			root->pos = pos;
			root->size = size;
			root->add(p);

#ifdef QUAD_TREE_DEBUG
			if (root->inside(p.pos))
			{
				assert(root->inside(p.pos));
				return false;
			}
#endif // QUAD_TREE_DEBUG

			return true;
		}

		// If the point is outside of the quad
		if (!root->inside(p.pos))
		{
			// Create quad backwards
			size_t count = 0;
			while (!root->inside(p.pos))
			{
				if (count++ > INFINITE_LOOP)
				{
					// Infinite loop
					assert(false && "Infinite loop");
					break;
				}
				Vec delta = p.pos - root->pos;
				Vec dir = {math_signalt(delta.x), math_signalt(delta.y)};
				Vec pos = root->pos;
				Vec size = root->size * (NumType)2;
				pos.x += math_min(dir.x, (NumType)0) * root->size.x;
				pos.y += math_min(dir.y, (NumType)0) * root->size.y;
				Node *newRoot = new Node{pos, size};

				Vec delta2 = (root->pos + root->size / (NumType)2) - newRoot->pos;
				size_t index = (size_t)(delta2.x / root->size.x) + (size_t)(delta2.y / root->size.y) * root->cx;

				if (root->is_leaf() && root->empty())
					delete root;
				else
				{
					newRoot->next[index] = root;
					root->parent = newRoot;
				}

				root = newRoot;
#ifdef QUAD_TREE_INFO
				printNode("A", root);
#endif // QUAD_TREE_INFO
			}
		}

		Node *node = root, *parent = nullptr;
		while (node && !node->is_leaf())
		{
			parent = node;
			node = node->get(p.pos);
		}

		if (!node)
		{
			if (parent)
				node = add_or_get_quad(parent, p.pos);
			else if (parent && !node)
			{
				if (parent && !node)
				{
#ifdef QUAD_TREE_DEBUG
					assert(parent && !node);
#endif
					return false;
				}
			}
		}

		node->add(p);

		// Split rects
		if (node->points.size() > maxPoints &&
			node->size.x > minSize.x &&
			node->size.y > minSize.y)
		{
			while (node->points.size())
			{
				Point &p = node->points.back();
				add_or_get_quad(node, p.pos)->add(p);
				node->points.pop_back();
			}
		}

#ifdef QUAD_TREE_DEBUG
		assert(node->inside(p.pos));
#endif // QUAD_TREE_DEBUG

		return true;
	}

	template <typename U, typename = std::enable_if<std::is_base_of<Type, U>::value>>
	inline bool
	remove(const Vec &vec, const U &u)
	{
		const Type t = dynamic_cast<const Type>(u);
		auto pair = get_pair(vec, t);
		return remove(pair, t);
	}

	inline bool
	remove(const Vec &vec)
	{
		auto pair = get_pair(vec);
		return remove(pair);
	}

	bool remove(const t_pair &pair, const Type &origin = Type())
	{
		Node *node = pair.first;
		if (!node)
		{
#ifdef QUAD_TREE_DEBUG
			assert(node);
#endif

			return false;
		}

		auto &points = node->points;
		Point targetPoint = *pair.second;

		auto lastItr = std::remove(points.begin(), points.end(), targetPoint);
		if (lastItr == points.end())
		{
#ifdef QUAD_TREE_DEBUG
			assert(0);
#endif
			return false;
		}
		points.erase(lastItr, points.end());

		if (!node->empty())
		{
			return true;
		}

		Node *bottom = node;
		while (bottom && bottom->empty() && bottom->parent)
		{
			Node *parent = bottom->parent;
			assert(bottom->parent != bottom);
			if (bottom == root)
				root = nullptr;
			else if (parent)
			{
				for (size_t i = 0u; i < 4u; ++i)
					if (parent->next[i] == bottom)
						parent->next[i] = nullptr;
				assert(parent != parent->parent);
			}
			delete bottom;
			bottom = parent;
		}

		return true;
	}

	void destroy(Node *root)
	{
		std::stack<Node *> stack;
		stack.push(root);
		while (!stack.empty())
		{
			Node *node = stack.top();
			stack.pop();
			if (!node)
				continue;

			for (size_t i = 0u; i < 4u; ++i)
			{
				if (node->next[i])
					stack.push(node->next[i]);
			}

			delete node;
		}
	}

	bool move(const Vec &posOld, const Vec &posNew, const Type &type)
	{
		t_pair pair = get_pair(posOld, type);

		if (!pair.first)
		{
#ifdef QUAD_TREE_DEBUG
			assert(pair.first);
#endif
			return false;
		}

		if (pair.first->inside(posNew))
		{
			Point &p = *pair.second;
			p.pos = posNew;
		}
		else
		{
			Point point = *(pair.second);
			point.pos = posNew;
			return remove(pair) ? insert(point) : false;
		}

		return true;
	}

#define asd        \
	if (_tmpDebug) \
		LOG("%d ", _tmpDebug);

	t_pair get_pair(const Vec &vec)
	{
		if (!root)
			return {nullptr, t_point_itr{}};

		Node *n = root;
		while (n && !n->is_leaf())
			n = n->get(vec);

		if (!n)
			return {nullptr, t_point_itr{}};

		std::vector<Point> &points = n->points;

		typename std::vector<Point>::iterator itrBest;
		if (points.empty())
		{
			itrBest = points.end();
		}
		else
		{
			itrBest = points.begin();
			for (auto itr = points.begin() + 1; itr != points.end(); ++itr)
			{
				itrBest = vec_cmp_dist(itrBest->pos, itr->pos, vec) ? itrBest : itr;
			}
		}

		if (itrBest != points.end() && n->inside(itrBest->pos))
		{
			return {n, itrBest};
		}
		else
		{
#ifdef QUAD_TREE_DEBUG
			assert(n->inside(itrBest->pos));
#endif
			return {nullptr, t_point_itr{}};
		}
	}

	bool _tmpDebug = 0;

	t_pair get_pair(const Vec &vec, const Type &type)
	{
		if (!root)
			return {nullptr, t_point_itr{}};

		Node *n = root;
		while (n && !n->is_leaf())
			n = n->get(vec);

		if (!n)
			return {nullptr, t_point_itr{}};

		std::vector<Point> &points = n->points;
		typename std::vector<Point>::iterator itrBest;

		/*
        if (points.size())
        {
            itrBest = points.begin();
            for (auto itr = points.begin() + 1; itr != points.end(); ++itr)
            {
                bool properA = itr->type != type,
                     properB = itrBest->type != type;

                if (!properA && !properB)
                    continue;
                if (properA && properB)
                    itrBest = vec_cmp_dist(itrBest->pos, itr->pos, vec) ? itrBest : itr;
                else
                    itrBest = (itr->type == type) ? itr : itrBest;
            }
            
        }
        
        if (points.empty() || itrBest->type != type)
        {
             itrBest = points.end();
        }
        */
		itrBest = std::find_if(
			points.begin(),
			points.end(),
			[&type](const Point &point) {
				return point.type == type;
			});

#ifdef QUAD_TREE_DEBUG
		assert(n->inside(itrBest->pos));
#endif

		if (itrBest != points.end() && n->inside(itrBest->pos))
			return {n, itrBest};
		else
			return {nullptr, t_point_itr{}};
	}

	inline Node *add_or_get_quad(Node *parent, const Vec &vec)
	{
		Node *node = parent->get(vec);
		if (!node)
			node = parent->create(vec);
		return node;
	}

	template <typename T>
	static T node_dist(Node *node, const t_vec<T> &vec)
	{
		t_vec<T> p1 = (t_vec<T>)node->pos;
		t_vec<T> p2 = (t_vec<T>)(node->pos + node->size);
		t_vec<T> d1 = vec - p1;
		t_vec<T> d2 = vec - p2;

		if (d1.x * d2.x < 0)
		{
			if (d1.y * d2.y < 0)
				return 0;
			return math_min(d1.y * d1.y, d2.y * d2.y);
		}
		if (d1.y * d2.y < 0)
			return math_min(d1.x * d1.x, d2.x * d2.x);

		return math_min(
			math_min(vec_distsq<NumType>(vec, p1), vec_distsq<NumType>(vec, {p1.x, p2.y})),
			math_min(vec_distsq<NumType>(vec, p2), vec_distsq<NumType>(vec, {p2.x, p1.y})));
	}

  public:
	// Once found all closest points stop
	void hard_optimization(bool enable)
	{
		this->hardopt = enable;
	}

	// K nearest neighbors algorithms

	/*
	Simplest implementation, most efficient
	Does not iterate through neighbors of higher levels
	if found any/enough close points
	
	Source:
	https://observablehq.com/@llb4ll/k-nearest-neighbor-search-using-d3-quadtrees
	*/
	template <
		class Condition = t_def_condition,
		class Comparator = t_def_comparator>
	void
	k_nearest_rec(
		std::vector<Point *> &result,
		const FVec &vec,
		size_t limit,
		t_float maxDistance = std::numeric_limits<t_float>::max(),
		const Condition &condition =
			DEFAULT_CONDITION,
		const Comparator &comparator =
			DEFAULT_COMPARATOR)
	{
		std::deque<DistPair<Node *>> best;
		best.push_back({node_dist(root, vec), root});
		t_float furthest = (t_float)0;

		t_float maxDistSq = MAX_FLOAT;
		if (maxDistance != MAX_FLOAT)
			maxDistSq = maxDistance * maxDistance;

		while (!best.empty())
		{
			// For higher perdormance and lower accurecy
			if (hardopt)
			{
				if (furthest > maxDistSq || result.size() >= limit)
					break;
			}

			Node *back = best.front().data;
			best.pop_front();
			back->_checked = 1;

			// If all remaining nodes are further than the
			// furthest found point there's no reason to
			// cheak them
			if (result.size() > 0 && result.size() == limit && back)
			{
				
				t_float nd = node_dist(back, vec);
				
				t_float n2 = vec_distsq(result.back()->pos, vec);
				
				if (nd > n2)
					break;
			}

			if (node_dist(back, vec) >= maxDistSq)
			{
				break;
			}

			if (back->is_leaf())
			{
				for (auto itr = back->points.begin(); itr != back->points.end(); itr++)
				{
					Point *p = &*itr;
					// NumType backDist;
					if (condition(p->pos, p->type, vec_distsq(vec, p->pos)))
					{
						auto lowerLambda = [&vec, &comparator](Point *a, Point *b) {
							return comparator(
								vec,
								a->pos,
								a->type,
								b->pos,
								b->type);
						};
						auto itrLower = std::lower_bound(
							result.begin(),
							result.end(),
							p,
							lowerLambda);

						// Calculate distance between current node to best node
						// If there's no best node, declare itself as best node
						// backDist = (result.size() ? vec_distsq(result.back()->pos, vec) : MAX_NUM);

						NumType fdist = vec_distsq(p->pos, vec);
						// Insert (and automaticly order by distance to "vec")
						result.insert(itrLower, p);

						// Remove container tail if need
						if (result.size() > limit || fdist > maxDistSq)
						{
							if (furthest < fdist)
							{
								furthest = fdist;
							}
							result.pop_back();
						}
					}
				}

				continue;
			}

			// else...
			for (size_t i = 0u; i < (size_t)back->count; ++i)
			{
				Node *n = back->next[i];
				if (!n)
					continue;
				DistPair<Node *> p =
					DistPair<Node *>{node_dist(n, vec), n};
				auto itr = std::lower_bound(
					best.begin(),
					best.end(),
					p);
				best.insert(itr, p);
			}
		}
	}

	// Insertors

	template <class Inserter,
			  typename U = Type,
			  class Condition = t_def_condition,
			  class Comparator = t_def_comparator>
	void nearest_k_radius_insertor(
		const FVec &vec,
		size_t limit,
		t_float maxDistance,
		Inserter inserter,
		const Condition &condition =
			DEFAULT_CONDITION,
		const Comparator &comparator =
			DEFAULT_COMPARATOR)
	{
		if (!root)
			return;
		std::vector<Point *> result;
		result.reserve(limit);
		k_nearest_rec(
			result,
			vec,
			limit,
			maxDistance,
			condition,
			comparator);
		for (auto itr = result.begin(); itr != result.end(); ++itr)
			inserter++ = (U)(*itr)->type;
	}

	template <class Inserter,
			  typename U = Type,
			  class Condition = t_def_condition,
			  class Comparator = t_def_comparator>
	void nearest_k_insertor(
		const FVec &vec,
		size_t limit,
		Inserter inserter,
		const Condition &condition =
			DEFAULT_CONDITION,
		const Comparator &comparator =
			DEFAULT_COMPARATOR)
	{
		nearest_k_radius_insertor<Inserter, U, Condition, Comparator>(
			vec,
			limit,
			MAX_FLOAT,
			inserter,
			condition,
			comparator);
	}

	template <class Inserter,
			  typename U = Type,
			  class Condition = t_def_condition,
			  class Comparator = t_def_comparator>
	void nearest_radius_insertor(
		const FVec &vec,
		t_float maxDistance,
		Inserter inserter,
		const Condition &condition =
			DEFAULT_CONDITION,
		const Comparator &comparator =
			DEFAULT_COMPARATOR)
	{
		nearest_k_radius_insertor<Inserter, U, Condition, Comparator>(
			vec,
			MAX_NUM,
			maxDistance,
			inserter,
			condition,
			comparator);
	}
	/*
	template <class Inserter, typename U = Type, class Condition = t_def_condition, class Comparator = t_def_comparator>
	void nearest_radius_insertor(
		const FVec &vec,
		NumType maxDistance,
		Inserter inserter,
		const Condition& condition = DEFAULT_CONDITION,
		const Comparator& comparator = DEFAULT_COMPARATOR)
	{
		nearest_k_radius_insertor<Inserter, U, Condition, Comparator>(
			vec,
			MAX_SIZE,
			maxDistance,
			inserter,
			condition,
			comparator);
	}
	
    template <class Inserter, class Condition = t_def_condition, class Comparator = t_def_comparator>
    void k_nearest_iterator(
        const Vec &vec,
        size_t limit,
        Iterator itrStart,
        Iterator itrEnd,
        Condition condition = DEFAULT_CONDITION,
        Comparator comparator = DEFAULT_COMPARATOR)
    {
        if (!root)
            return;
        std::list<Point *> result;
        k_nearest_rec(result, vec, limit, -1, condition, comparator);
        Iterator outItr = itrStart;
        for (auto itr = result.begin(); itr != result.end(); ++itr)
        {
            if (outItr == itrEnd)
                break;
            *outItr = (*itr)->type;
            outItr++;
        }
    }

    template <class Iterator, typename U = Type, class Condition = t_def_condition, class Comparator = t_def_comparator>
    void nearest_radius_iterator(
        const Vec &vec,
        NumType maxDistance,
        Iterator itrStart,
        Iterator itrEnd,
        Condition condition = DEFAULT_CONDITION,
        Comparator comparator = DEFAULT_COMPARATOR)
    {
        if (!root)
            return;
        std::list<Point *> result;
        k_nearest_rec(result, vec, -1, maxDistance, condition, comparator);
        Iterator outItr = itrStart;
        for (auto itr = result.begin(); itr != result.end(); ++itr)
        {
            if (outItr == itrEnd)
                break;
            *outItr = (U)(*itr)->type;
            outItr++;
        }
    }

    template <class Condition = t_def_condition, class Comparator = t_def_comparator>
    bool nearest(
        const Vec &vec,
        Type &typeOut,
        Condition condition = DEFAULT_CONDITION,
        Comparator comparator = DEFAULT_COMPARATOR)
    {
        bool fail = !root;

        if (!fail)
        {
            std::list<DistPair<Point *>> result;
            k_nearest_rec(result, vec, 1, -1, condition, comparator);
            if (result.size())
            {
                typeOut = result.front().data->type;
                return true;
            }
            else
                fail = true;
        }

        if (fail)
        {
            typeOut = Type();
            return false;
        }
    }
*/

	inline Iterator begin()
	{
		return Iterator(root);
	}

	inline Iterator end()
	{
		return Iterator(nullptr);
	}
};

#endif // GAME_QUAD_TREE
