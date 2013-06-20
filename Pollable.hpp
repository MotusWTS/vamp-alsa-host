#ifndef POLLABLE_HPP
#define POLLABLE_HPP

/* a quasi-interface to allow an object to participate in polling by a VampAlsaHost 
   all Pollable objects created become part of a set indexed by string labels, and
   each one has a set of FDs which can participate in polling.  Participation can
   be enabled and disabled.
*/

#include <string>
#include <stdexcept>
#include <stdint.h>
#include <boost/circular_buffer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/make_shared.hpp>

using boost::make_shared;
using boost::shared_ptr;
using boost::weak_ptr;
using boost::static_pointer_cast;

#include "VampAlsaHost.hpp"

class Pollable;
typedef std::map < std::string, shared_ptr < Pollable > > PollableSet;

class Pollable {
public:
  /* class members */

  static const unsigned DEFAULT_OUTPUT_BUFFER_SIZE = 16384; // default size of output buffer; subclasses may request larger 

  static PollableSet pollables; // map of Pollables, indexed by label, values are shared pointers; this owns its objects
  static void remove(const string& label);
  static Pollable * lookupByName (const std::string& label);
  static shared_ptr < Pollable > lookupByNameShared (const std::string& label);
  static void requestPollFDRegen();
  static int poll(int timeout); // do one round of polling; return 0 on okay; errno otherwise

protected:
  static std::vector <struct pollfd> allpollfds; // in same order as pollables, but some pollables may have 0 or more than 1 FD
  static PollableSet deferred_removes;
  static bool regen_pollfds;
  static bool have_deferrals;
  static bool doing_poll;
  static void doDeferrals();  
  static void regenFDs();

  /* instance members */

public:

  Pollable(const string & label);

  virtual ~Pollable();

  string label;
  virtual string toJSON() = 0;
  virtual bool queueOutput(const char * p, uint32_t len, void * meta = 0);
  virtual bool queueOutput(std::string &str, void * meta = 0) {return queueOutput(str.data(), str.length(), meta);};
  int writeSomeOutput(int maxBytes);

  short & eventsOf(int offset = 0); // reference to the events field for a pollfd

  virtual int getNumPollFDs() = 0;                      // return number of fds used by this Pollable (negative means error)
  virtual int getPollFDs (struct pollfd * pollfds) = 0; // copy pollfds for this Pollable to the location specified (return non-zero on error)
  // (i.e. this reports pollable fds and the pollfd "events" field for this object)
  virtual int getOutputFD() = 0; // return the output pollfd, if applicable
  virtual void handleEvents (struct pollfd *pollfds, bool timedOut, double timeNow) {}; // handle possible event(s) on the fds for this Pollable
  virtual int start(double timeNow) = 0;
  virtual void stop(double timeNow) = 0;

protected:
  int indexInPollFD;  // index of first FD in class pollfds vector (< 0 means not in pollfd vector)
  struct pollfd pollfd;

  boost::circular_buffer < char > outputBuffer;
  bool outputPaused;
};

#endif /* POLLABLE_HPP */
