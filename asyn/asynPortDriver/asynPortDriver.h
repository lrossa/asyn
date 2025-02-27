#ifndef asynPortDriver_H
#define asynPortDriver_H

#include <vector>
#include <string>

#include <epicsTypes.h>
#include <epicsMutex.h>
#include <epicsThread.h>

#include <asynStandardInterfaces.h>
#include <asynParamSet.h>
#include <asynParamType.h>
#include <paramErrors.h>

class paramList;

ASYN_API void* findAsynPortDriver(const char *portName);
typedef void (*userTimeStampFunction)(void *userPvt, epicsTimeStamp *pTimeStamp);

#ifdef __cplusplus

/** Masks for each of the asyn standard interfaces */
#define asynCommonMask          0x00000001
#define asynDrvUserMask         0x00000002
#define asynOptionMask          0x00000004
#define asynInt32Mask           0x00000008
#define asynUInt32DigitalMask   0x00000010
#define asynFloat64Mask         0x00000020
#define asynOctetMask           0x00000040
#define asynInt8ArrayMask       0x00000080
#define asynInt16ArrayMask      0x00000100
#define asynInt32ArrayMask      0x00000200
#define asynFloat32ArrayMask    0x00000400
#define asynFloat64ArrayMask    0x00000800
#define asynGenericPointerMask  0x00001000
#define asynEnumMask            0x00002000
#define asynInt64Mask           0x00004000
#define asynInt64ArrayMask      0x00008000

class callbackThread;

/** Base class for asyn port drivers; handles most of the bookkeeping for writing an asyn port driver
  * with standard asyn interfaces and a parameter library.
  *
  * **Destruction and cleanup**
  *
  * Historically, drivers were never destroyed. There are several reasons for
  * this, an important one being that proper cleanup requires cooperation from
  * the underlying system and the entire class hierarchy. That said, releasing
  * resources is important in many cases, and is not too difficult if the rules
  * listed below are followed by classes deriving from `asynPortDriver`:
  *
  * 1. Pass the `ASYN_DESTRUCTIBLE` flag to the constructor of `asynPortDriver`.
  *    This will ensure `asynManager` destroys your driver on process exit by
  *    first calling `shutdownPortDriver()`, then deleting.
  *
  * 2. To release resources that are private to your derived class, do so in the
  *    destructor. Remember, however, that no code from classes deriving from
  *    yours may run at this point, because the object is already partly
  *    destroyed. For example, virtual functions will behave as if not
  *    overriden, and threads spawned by derived classes must already be
  *    stopped.
  *
  * 3. To use functionality that requires an intact object, release resources by
  *    overriding the `shutdownPortDriver()` function. A possible example is
  *    stopping data acquisition, which may involve functionality implemented in
  *    a derived class. On process exit, `shutdownPortDriver()` will be called
  *    before the destructors are executed.
  *
  * 4. Your overriden `shutdownPortDriver()` must call the base class
  *    implementation.
  *
  * 5. When deleting a driver instance directly (e.g., in your test code),
  *    always call `shutdownPortDriver()` first.
  *
  * To implement the above rules, you can use the following template:
  *
  *     class myDriver : public asynPortDriver {
  *     public:
  *         myDriver(const char *portName, ...) :
  *               asynPortDriver(portName, ..., ASYN_DESTRUCTIBLE, ...) {
  *             // Your driver code.
  *         }
  *
  *         void shutdownPortDriver() {
  *             // Stop threads, you may use virtual functions.
  *
  *             // Don't forget to call the base class function.
  *             baseDriver::shutdownPortDriver();
  *         }
  *
  *         ~myDriver() {
  *             // Deallocate resources, don't use virtual functions.
  *         }
  *     };
  */
class ASYN_API asynPortDriver {
public:
    asynPortDriver(asynParamSet* paramSet,
                   const char *portName, int maxAddr, int interfaceMask, int interruptMask,
                   int asynFlags, int autoConnect, int priority, int stackSize);
    asynPortDriver(const char *portName, int maxAddr, int interfaceMask, int interruptMask,
                   int asynFlags, int autoConnect, int priority, int stackSize);
    asynPortDriver(const char *portName, int maxAddr, int paramTableSize, int interfaceMask, int interruptMask,
                   int asynFlags, int autoConnect, int priority, int stackSize) EPICS_DEPRECATED;
    virtual ~asynPortDriver();
    virtual asynStatus lock();
    virtual asynStatus unlock();
    virtual asynStatus getAddress(asynUser *pasynUser, int *address);
    virtual asynStatus parseAsynUser(asynUser *pasynUser, int *reason, int *address, const char **paramName);
    virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus readInt64(asynUser *pasynUser, epicsInt64 *value);
    virtual asynStatus writeInt64(asynUser *pasynUser, epicsInt64 value);
    virtual asynStatus readUInt32Digital(asynUser *pasynUser, epicsUInt32 *value, epicsUInt32 mask);
    virtual asynStatus writeUInt32Digital(asynUser *pasynUser, epicsUInt32 value, epicsUInt32 mask);
    virtual asynStatus setInterruptUInt32Digital(asynUser *pasynUser, epicsUInt32 mask, interruptReason reason);
    virtual asynStatus clearInterruptUInt32Digital(asynUser *pasynUser, epicsUInt32 mask);
    virtual asynStatus getInterruptUInt32Digital(asynUser *pasynUser, epicsUInt32 *mask, interruptReason reason);
    virtual asynStatus getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high);
    virtual asynStatus getBounds64(asynUser *pasynUser, epicsInt64 *low, epicsInt64 *high);
    virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars,
                                        size_t *nActual, int *eomReason);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars,
                                        size_t *nActual);
    virtual asynStatus flushOctet(asynUser *pasynUser);
    virtual asynStatus setInputEosOctet(asynUser *pasynUser, const char *eos, int eosLen);
    virtual asynStatus getInputEosOctet(asynUser *pasynUser, char *eos, int eosSize, int *eosLen);
    virtual asynStatus setOutputEosOctet(asynUser *pasynUser, const char *eos, int eosLen);
    virtual asynStatus getOutputEosOctet(asynUser *pasynUser, char *eos, int eosSize, int *eosLen);
    virtual asynStatus readInt8Array(asynUser *pasynUser, epicsInt8 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus writeInt8Array(asynUser *pasynUser, epicsInt8 *value,
                                        size_t nElements);
    virtual asynStatus doCallbacksInt8Array(epicsInt8 *value,
                                        size_t nElements, int reason, int addr);
    virtual asynStatus readInt16Array(asynUser *pasynUser, epicsInt16 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus writeInt16Array(asynUser *pasynUser, epicsInt16 *value,
                                        size_t nElements);
    virtual asynStatus doCallbacksInt16Array(epicsInt16 *value,
                                        size_t nElements, int reason, int addr);
    virtual asynStatus readInt32Array(asynUser *pasynUser, epicsInt32 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus writeInt32Array(asynUser *pasynUser, epicsInt32 *value,
                                        size_t nElements);
    virtual asynStatus doCallbacksInt32Array(epicsInt32 *value,
                                        size_t nElements, int reason, int addr);
    virtual asynStatus readInt64Array(asynUser *pasynUser, epicsInt64 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus writeInt64Array(asynUser *pasynUser, epicsInt64 *value,
                                        size_t nElements);
    virtual asynStatus doCallbacksInt64Array(epicsInt64 *value,
                                        size_t nElements, int reason, int addr);
    virtual asynStatus readFloat32Array(asynUser *pasynUser, epicsFloat32 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus writeFloat32Array(asynUser *pasynUser, epicsFloat32 *value,
                                        size_t nElements);
    virtual asynStatus doCallbacksFloat32Array(epicsFloat32 *value,
                                        size_t nElements, int reason, int addr);
    virtual asynStatus readFloat64Array(asynUser *pasynUser, epicsFloat64 *value,
                                        size_t nElements, size_t *nIn);
    virtual asynStatus writeFloat64Array(asynUser *pasynUser, epicsFloat64 *value,
                                        size_t nElements);
    virtual asynStatus doCallbacksFloat64Array(epicsFloat64 *value,
                                        size_t nElements, int reason, int addr);
    virtual asynStatus readGenericPointer(asynUser *pasynUser, void *pointer);
    virtual asynStatus writeGenericPointer(asynUser *pasynUser, void *pointer);
    virtual asynStatus doCallbacksGenericPointer(void *pointer, int reason, int addr);
    virtual asynStatus readOption(asynUser *pasynUser, const char *key, char *value, int maxChars);
    virtual asynStatus writeOption(asynUser *pasynUser, const char *key, const char *value);
    virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], size_t nElements, size_t *nIn);
    virtual asynStatus writeEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], size_t nElements);
    virtual asynStatus doCallbacksEnum(char *strings[], int values[], int severities[], size_t nElements, int reason, int addr);
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo,
                                     const char **pptypeName, size_t *psize);
    virtual asynStatus drvUserGetType(asynUser *pasynUser,
                                        const char **pptypeName, size_t *psize);
    virtual asynStatus drvUserDestroy(asynUser *pasynUser);
    virtual void report(FILE *fp, int details);
    virtual asynStatus connect(asynUser *pasynUser);
    virtual asynStatus disconnect(asynUser *pasynUser);

    virtual asynStatus createParam(          const char *name, asynParamType type, int *index);
    virtual asynStatus createParam(int list, const char *name, asynParamType type, int *index);
    virtual asynStatus createParams();
    virtual asynStatus getNumParams(          int *numParams);
    virtual asynStatus getNumParams(int list, int *numParams);
    virtual asynStatus findParam(          const char *name, int *index);
    virtual asynStatus findParam(int list, const char *name, int *index);
    virtual asynStatus getParamName(          int index, const char **name);
    virtual asynStatus getParamName(int list, int index, const char **name);
    virtual asynStatus getParamType(          int index, asynParamType *type);
    virtual asynStatus getParamType(int list, int index, asynParamType *type);
    virtual asynStatus setParamStatus(          int index, asynStatus status);
    virtual asynStatus setParamStatus(int list, int index, asynStatus status);
    virtual asynStatus getParamStatus(          int index, asynStatus *status);
    virtual asynStatus getParamStatus(int list, int index, asynStatus *status);
    virtual asynStatus setParamAlarmStatus(          int index, int status);
    virtual asynStatus setParamAlarmStatus(int list, int index, int status);
    virtual asynStatus getParamAlarmStatus(          int index, int *status);
    virtual asynStatus getParamAlarmStatus(int list, int index, int *status);
    virtual asynStatus setParamAlarmSeverity(          int index, int severity);
    virtual asynStatus setParamAlarmSeverity(int list, int index, int severity);
    virtual asynStatus getParamAlarmSeverity(          int index, int *severity);
    virtual asynStatus getParamAlarmSeverity(int list, int index, int *severity);
    virtual void       reportSetParamErrors(asynStatus status, int index, int list, const char *functionName);
    virtual void       reportGetParamErrors(asynStatus status, int index, int list, const char *functionName);
    virtual asynStatus setIntegerParam(          int index, int value);
    virtual asynStatus setIntegerParam(int list, int index, int value);
    virtual asynStatus setInteger64Param(          int index, epicsInt64 value);
    virtual asynStatus setInteger64Param(int list, int index, epicsInt64 value);
    virtual asynStatus setUIntDigitalParam(          int index, epicsUInt32 value, epicsUInt32 valueMask);
    virtual asynStatus setUIntDigitalParam(int list, int index, epicsUInt32 value, epicsUInt32 valueMask);
    virtual asynStatus setUIntDigitalParam(          int index, epicsUInt32 value, epicsUInt32 valueMask, epicsUInt32 interruptMask);
    virtual asynStatus setUIntDigitalParam(int list, int index, epicsUInt32 value, epicsUInt32 valueMask, epicsUInt32 interruptMask);
    virtual asynStatus setUInt32DigitalInterrupt(          int index, epicsUInt32 mask, interruptReason reason);
    virtual asynStatus setUInt32DigitalInterrupt(int list, int index, epicsUInt32 mask, interruptReason reason);
    virtual asynStatus clearUInt32DigitalInterrupt(         int index, epicsUInt32 mask);
    virtual asynStatus clearUInt32DigitalInterrupt(int list, int index, epicsUInt32 mask);
    virtual asynStatus getUInt32DigitalInterrupt(          int index, epicsUInt32 *mask, interruptReason reason);
    virtual asynStatus getUInt32DigitalInterrupt(int list, int index, epicsUInt32 *mask, interruptReason reason);
    virtual asynStatus setDoubleParam(          int index, double value);
    virtual asynStatus setDoubleParam(int list, int index, double value);
    virtual asynStatus setStringParam(          int index, const char *value);
    virtual asynStatus setStringParam(int list, int index, const char *value);
    virtual asynStatus setStringParam(          int index, const std::string& value);
    virtual asynStatus setStringParam(int list, int index, const std::string& value);
    virtual asynStatus getIntegerParam(          int index, epicsInt32 * value);
    virtual asynStatus getIntegerParam(int list, int index, epicsInt32 * value);
    virtual asynStatus getInteger64Param(          int index, epicsInt64 * value);
    virtual asynStatus getInteger64Param(int list, int index, epicsInt64 * value);
    virtual asynStatus getUIntDigitalParam(          int index, epicsUInt32 *value, epicsUInt32 mask);
    virtual asynStatus getUIntDigitalParam(int list, int index, epicsUInt32 *value, epicsUInt32 mask);
    virtual asynStatus getDoubleParam(          int index, double * value);
    virtual asynStatus getDoubleParam(int list, int index, double * value);
    virtual asynStatus getStringParam(          int index, int maxChars, char *value);
    virtual asynStatus getStringParam(int list, int index, int maxChars, char *value);
    virtual asynStatus getStringParam(          int index, std::string& value);
    virtual asynStatus getStringParam(int list, int index, std::string& value);
    virtual asynStatus callParamCallbacks();
    virtual asynStatus callParamCallbacks(          int addr);
    virtual asynStatus callParamCallbacks(int list, int addr);
    virtual asynStatus updateTimeStamp();
    virtual asynStatus updateTimeStamp(epicsTimeStamp *pTimeStamp);
    virtual asynStatus getTimeStamp(epicsTimeStamp *pTimeStamp);
    virtual asynStatus setTimeStamp(const epicsTimeStamp *pTimeStamp);
    asynStandardInterfaces *getAsynStdInterfaces();
    virtual void reportParams(FILE *fp, int details);
    virtual void shutdownPortDriver();
    bool needsShutdown();

    char *portName;         /**< The name of this asyn port */

    int maxAddr;            /**< The maximum asyn address (addr) supported by this driver */
    void callbackTask();

protected:
    void initialize(const char *portNameIn, int maxAddrIn, int interfaceMask, int interruptMask, int asynFlags,
                    int autoConnect, int priority, int stackSize);

    asynParamSet* paramSet;
    asynUser *pasynUserSelf;    /**< asynUser connected to ourselves for asynTrace */
    asynStandardInterfaces *pasynStdInterfaces;   /**< The asyn interfaces this driver implements */
    asynStandardInterfaces &asynStdInterfaces;   /**< Back-compat alias */

private:
    static void exceptionHandler(asynUser *pasynUser, asynException exception);

    std::vector<paramList*> params;
    paramList *getParamList(int list);
    epicsMutexId mutexId;
    char *inputEosOctet;
    int inputEosLenOctet;
    char *outputEosOctet;
    int outputEosLenOctet;
    callbackThread *cbThread;
    int shutdownNeeded;  // atomic!
    template <typename epicsType, typename interruptType>
        asynStatus doCallbacksArray(epicsType *value, size_t nElements,
                                    int reason, int address, void *interruptPvt);

    friend class paramList;
    friend class callbackThread;
};

class callbackThread: public epicsThreadRunable {
public:
    callbackThread(asynPortDriver *portDriver);
    ~callbackThread();
    void run();
private:
    epicsThread *pThread;
    asynPortDriver *pPortDriver;
    epicsEvent shutdown;
    epicsEvent doneEvent;
};

/** Utility function that returns a pointer to an asynPortDriver derived class object from its name */
template <class T> T* findDerivedAsynPortDriver(const char* portName)
{
  // findAsynPortDriver returns a void pointer that was cast from an asynPortDriver pointer
  asynPortDriver* apd = (asynPortDriver*) findAsynPortDriver(portName);
  if (!apd) return NULL;

  // Downcast asynPortDriver pointer to T pointer - this requires pointer offsetting
  T* pC = dynamic_cast<T*>(apd);
  return pC;
}

#endif /* cplusplus */

#endif
