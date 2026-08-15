#ifndef GAME_TIMER
#define GAME_TIMER

// My most complex class yet

#include <cassert>
#include <type_traits>
#include <vector>
#include <memory>

template <typename U = float>
class GameTimer
{
  static_assert(!std::is_unsigned<U>::value, "This class uses number subtraction.");

  public:

      GameTimer()
      {

      }

    void init(U time)
    {
        m_start = time;
        bInit = true;
        m_stopped = false;

        if (m_saveTime != -1)
        {
            U timePassed = time - m_saveTime;
            m_start += timePassed;
            m_suspend_start += timePassed;
        }
    }

    /**
     * Wraps the "next_surplus" function
     */
    bool operator()(U time)
    {
        return next_surplus(time);
    }
    
    /**
     * Check if stopwatch finished, and if the countdown was passed
     * multiple times. Instead of reseting add to the time length and
     * return true for every function call for every pass of the timer.
     * I don't know how to describe it wtf, like if the 30 sec passed
     * but the coundown is 10 sec in a while look the function will
     * return true three times and then reset as it returns false.
     */
    bool next_surplus(U time)
    {
        if (m_stopped || m_paused)
        	return false;
        
        if (!bInit)
        {
            bInit = true;
            m_start = time;
            return true;
        }
        
        if (m_length == (U)0 || m_start == (U)-1)
            return false;
        
        int passes = (int)((time - m_start) / m_length);
        if (passes == 0) {
          return false;
        }
        else if (passes == 1) {
          reset(time);
          return true;
        }
        else if (passes > 1) {
          m_start += m_length;
          return true;
        }
        else {
          reset(time);
         //  assert(false && "Timing on timer is off, reset cannot be called ahead of time.\n");
          return false;
        }
    }

    U time_passed(U current)
    {
        return current - m_start;
    }
    
    bool finished(U time)
    {
        if (m_length == (U)0 || m_start == (U)-1)
            return false;
        return time - m_start >= m_length;
    }
    
    void reset(U time)
    {
        m_start = time;
        m_stopped = false;
    }

    float progress(U time)
    {
      if (m_stopped)
      	return 0.0f;
      float x = (time - m_start) / m_length;
      x = (x < 0.0f) ? 0.0f : (x > 1.0f ? 1.0f : x);
      return x;
    }

    void set_length(U length)
    {
        m_length = length;
    }
    
    bool initialized()
    {
        return m_length != (U)0 && m_start != (U)-1 && bInit;
    }
    
    void stop()
    {
    	m_stopped = true;
    }

    // Stops the timer, so when it resumed no time should pass
    void suspend(U time)
    {
        if (m_paused)
            return;
        this->m_suspend_start = time;
        m_paused = true;
    }

    // The opposite of suspend
    void resume(U time)
    {
        if (!m_paused)
            return;
        m_paused = false;
        U suspendLen = time - this->m_suspend_start;
        if (suspendLen < 0)
            return;
        m_start += suspendLen;
    }

    U m_suspend_start = (U)0;
    U m_length = (U)0;
    U m_start = (U)-1;
    U m_saveTime = (U)-1;
    bool bInit = false;
    bool m_stopped = false;
    bool m_paused = false;
};

template <typename U = float>
class GameTimerManager
{
public:
    typedef std::shared_ptr<GameTimer<U>> t_timer_ptr;

    GameTimerManager()
    {
    }

    static inline t_timer_ptr make_independent_timer(U time = (U)0)
    {
        t_timer_ptr timerPtr;
        timerPtr = std::make_shared<GameTimer<U>>();
        timerPtr.get()->init(time);
        return timerPtr;
    }

    t_timer_ptr make_new(U time)
    {
        t_timer_ptr timerPtr;
        timerPtr = make_independent_timer(time);
        m_data.push_back(timerPtr);
        return timerPtr;
    } 
    
    t_timer_ptr make_new()
    {
        t_timer_ptr timerPtr;
        timerPtr = std::make_shared< GameTimer<U>>();
        m_data.push_back(timerPtr);
        return timerPtr;
    }


    void add_new(t_timer_ptr& timer)
    {
        m_data.push_back(timer);
    }

    void stop_all(U time)
    {
        for (t_timer_ptr& x : m_data)
        {
            x.get()->suspend(time);
        }
    }

    void resume_all(U time)
    {
        for (t_timer_ptr& x : m_data)
        {
            x.get()->resume(time);
        }
    }

private:
    std::vector< t_timer_ptr > m_data;

};

#endif // GAME_TIMER