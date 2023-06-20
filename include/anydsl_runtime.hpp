#ifndef ANYDSL_RUNTIME_HPP
#define ANYDSL_RUNTIME_HPP

#include "anydsl_runtime.h"

#ifndef ANYDSL_NAMESPACE
#define ANYDSL_NAMESPACE anydsl
#endif

namespace ANYDSL_NAMESPACE {

// TODO: This is only a subset of the interface
// TODO: Properly handle errors

enum class Platform : int32_t {
    Host   = AnyDSL_DEVICE_HOST,
    Cuda   = AnyDSL_DEVICE_CUDA,
    OpenCL = AnyDSL_DEVICE_OPENCL,
    HSA    = AnyDSL_DEVICE_HSA
};

class Device {
public:
    Device()
        : mNum(-1)
        , mHandle(AnyDSL_NULL_HANDLE)
        , mIsHost(false)
    {
    }

    Device(Platform platform, int32_t num = 0)
        : mNum(num)
        , mIsHost(platform == Platform::Host)
    {
        AnyDSLGetDeviceRequest info = {
            AnyDSL_STRUCTURE_TYPE_GET_DEVICE_REQUEST,
            nullptr,
            (AnyDSLDeviceType)platform,
            (uint32_t)num
        };

        anydslGetDevice(&info, &mHandle);
    }

    inline bool sync()
    {
        return anydslSynchronizeDevice(mHandle) == AnyDSL_SUCCESS;
    }

    inline AnyDSLDevice handle() const { return mHandle; }
    inline bool isHost() const { return mIsHost; }

protected:
    int32_t mNum;
    AnyDSLDevice mHandle;
    bool mIsHost;
};

template <typename T>
class Array {
public:
    Array()
        : mData(nullptr)
        , mSize(0)
        , mDevice()
        , mBuffer(AnyDSL_NULL_HANDLE)
    {
    }

    Array(int64_t size)
        : Array(Device(), size)
    {
    }

    Array(const Device& dev, int64_t size)
        : mData(nullptr)
        , mSize(0)
        , mDevice(dev)
        , mBuffer(AnyDSL_NULL_HANDLE)
    {
        allocate(size);
    }

    Array(Array&& other)
        : mData(other.mData)
        , mSize(other.mSize)
        , mDevice(other.mDevice)
        , mBuffer(other.mBuffer)
    {
        other.mData   = nullptr;
        other.mBuffer = AnyDSL_NULL_HANDLE;
    }

    Array& operator=(Array&& other)
    {
        deallocate();
        mDevice       = other.mDevice;
        mSize         = other.mSize;
        mData         = other.mData;
        mBuffer       = other.mBuffer;
        other.mData   = nullptr;
        other.mBuffer = AnyDSL_NULL_HANDLE;
        return *this;
    }

    Array(const Array&)            = delete;
    Array& operator=(const Array&) = delete;

    ~Array() { deallocate(); }

    T* begin() { return mData; }
    const T* begin() const { return mData; }

    T* end() { return mData + mSize; }
    const T* end() const { return mData + mSize; }

    T* data() { return mData; }
    const T* data() const { return mData; }

    int64_t size() const { return mSize; }
    const Device& device() const { return mDevice; }

    const T& operator[](int i) const { return mData[i]; }
    T& operator[](int i) { return mData[i]; }

    T* release()
    {
        T* ptr = mData;
        mData  = nullptr;
        mSize  = 0;
        return ptr;
    }

    inline AnyDSLBuffer handle() const { return mBuffer; }

protected:
    void allocate(int64_t size)
    {
        // Deallocate if already allocated
        deallocate();

        mSize = size;

        AnyDSLCreateBufferInfo info = {
            AnyDSL_STRUCTURE_TYPE_CREATE_BUFFER_INFO,
            nullptr,
            sizeof(T) * size,
            0
        };
        if (anydslCreateBuffer(mDevice.handle(), &info, &mBuffer) != AnyDSL_SUCCESS)
            return;

        AnyDSLGetBufferPointerInfo ptrInfo = {
            AnyDSL_STRUCTURE_TYPE_GET_BUFFER_POINTER_INFO,
            nullptr,
            0, // Will be set by the function
            0  // Will be set by the function
        };
        if (anydslGetBufferPointer(mBuffer, &ptrInfo) != AnyDSL_SUCCESS) {
            anydslDestroyBuffer(mBuffer);
            return;
        }

        mData = reinterpret_cast<T*>(ptrInfo.devicePointer);
    }

    void deallocate()
    {
        if (mData != nullptr) {
            anydslDestroyBuffer(mBuffer);
            mData   = nullptr;
            mBuffer = AnyDSL_NULL_HANDLE;
        }
    }

    T* mData;
    int64_t mSize;
    Device mDevice;
    AnyDSLBuffer mBuffer;
};

template <typename T>
void copy(const Array<T>& src, int64_t offset_src, Array<T>& dst, int64_t offset_dst, int64_t size)
{
    AnyDSLBufferCopy region = {
        offset_src, offset_dst, src.size() * sizeof(T)
    };
    anydslCopyBuffer(src.handle(), dst.handle(), 1, &region);
}

template <typename T>
void copy(const Array<T>& src, Array<T>& dst, int64_t size)
{
    copy(src, 0, dst, 0, size);
}

template <typename T>
void copy(const Array<T>& src, Array<T>& dst)
{
    copy(src, dst, src.size());
}

class Event {
public:
    inline Event()
        : mDevice()
    {
        create();
    }

    inline Event(const Device& device)
        : mDevice(device)
    {
        create();
    }

    inline ~Event()
    {
        destroy();
    }

    inline bool record()
    {
        return anydslRecordEvent(mEvent) == AnyDSL_SUCCESS;
    }

    inline bool wait()
    {
        return anydslSynchronizeEvent(mEvent) == AnyDSL_SUCCESS;
    }

    inline AnyDSLEvent handle() const { return mEvent; }

    inline static float elapsedTimeMS(const Event& start, const Event& end)
    {
        AnyDSLQueryEventInfo info = {
            AnyDSL_STRUCTURE_TYPE_QUERY_EVENT_INFO,
            nullptr,
            0.0f // Will be set by the function
        };
        AnyDSLResult res = anydslQueryEvent(start.handle(), end.handle(), &info);

        if (res != AnyDSL_SUCCESS)
            return -1;
        else
            return info.elapsedTimeMS;
    }

private:
    inline void create()
    {
        AnyDSLCreateEventInfo info = {
            AnyDSL_STRUCTURE_TYPE_CREATE_EVENT_INFO,
            nullptr
        };
        anydslCreateEvent(mDevice.handle(), &info, &mEvent);
    }

    inline void destroy()
    {
        anydslDestroyEvent(mEvent);
    }

    Device mDevice;
    AnyDSLEvent mEvent;
};

} // namespace ANYDSL_NAMESPACE

#endif // ANYDSL_RUNTIME_HPP