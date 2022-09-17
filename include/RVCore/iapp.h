#ifndef __IAPP__H__
#define __IAPP__H__

#include <chrono>
#include <string>

namespace rv
{
    class IApp
    {
	using highResClock = std::chrono::high_resolution_clock;
	using duration = highResClock::duration;

      private:
	const char* name;
	bool sleeping;
	double deltaTime;

	IApp() = delete;
	IApp(IApp&&) = delete;
	IApp(const IApp&) = delete;
	IApp& operator=(IApp&&) = delete;
	IApp& operator=(const IApp&) = delete;

      public:
	constexpr IApp(const char* name)
	    : name(name), sleeping(false), deltaTime(0.016), shouldSleep(false), shouldWakeUp(true),
	      shouldQuit(false){};
	~IApp() = default;

	void Run();
	const char* GetName() { return name; };

      protected:
	virtual void Setup() = 0;
	virtual void Awake() = 0;
	virtual void Update(double deltaTime) = 0;
	virtual void Sleep() = 0;
	virtual void Shutdown() = 0;

	bool shouldSleep;
	bool shouldWakeUp;
	bool shouldQuit;
    };

    inline void IApp::Run()
    {
	Setup();
	while (!shouldQuit)
	{
	    if (sleeping)
	    {
		if (shouldWakeUp)
		{
		    sleeping = false;
		    shouldWakeUp = false;
		    Awake();
		    continue;
		}
	    }
	    else
	    {
		if (shouldSleep)
		{
		    Sleep();
		    sleeping = true;
		    shouldSleep = false;
		    continue;
		}
	    }

	    highResClock::time_point start = highResClock::now();
	    Update(deltaTime);
	    highResClock::time_point stop = highResClock::now();

	    using ms = std::chrono::duration<double>;
	    deltaTime = std::chrono::duration_cast<ms>(stop - start).count();
	}
	Shutdown();
    }
} // namespace rv

#endif //!__IAPP__H__