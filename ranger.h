#ifndef __ranger_h
#define __ranger_h

//-----------------------------------------------------------------------------

#include <thread>
#include <mutex>

//-----------------------------------------------------------------------------

class Ranger {
public:
	/// Default constructor
	Ranger();

	/// Destructor
	virtual ~Ranger();

	/// Returns most recent range measurement in metres
	double getRange() const;

	/// Returns total number of range measurements made so far
	unsigned getCount() const;

	/// Wait until the range finder is ready
	bool initialise();

	/// Is the range finder ready for use?
	bool ready() const;

private:
	/// Worker thread
	void worker();

	/// Measures and returns range in metres. Will block while the ranging
	/// operation is in progress. May return zero in case of failure.
	double measureRange();

private:
	double  m_timeLastRun;	///< Time when getRange() was last called

	double	m_range;		///< Last range measurement
	unsigned m_count;		///< Number of range measurements so far

	bool	m_run;			///< Should thread continue to run?

	/// Thread used to take range measurements
	std::thread m_thread;

	/// Mutex to control access to the member variables
	mutable std::mutex m_mutex;
};

//-----------------------------------------------------------------------------

#endif//__ranger_h

