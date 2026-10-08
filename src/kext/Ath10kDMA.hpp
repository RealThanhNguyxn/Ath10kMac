#pragma once

#include <IOKit/IOLib.h>
#include <IOKit/IOBufferMemoryDescriptor.h>
#include <IOKit/IODMACommand.h>
#include <IOKit/IOMapper.h>
#include <IOKit/IOTypes.h>

#ifndef ATH10K_PAYLOAD_CACHE
#define ATH10K_PAYLOAD_CACHE 1
#endif

// Host-side DMA helper. The important distinction is between CPU physical
// memory and the device-visible 32-bit DMA address. IODMACommand may satisfy
// the 32-bit constraint through a mapper or through internal buffering. The
// caller therefore must explicitly synchronize CPU writes before device DMA
// and device writes before CPU reads.
class Ath10kDMABuffer
{
public:
    enum class Role { Ring, Payload };
    bool allocate(UInt64 size, UInt64 alignment, IOMapper *mapper = nullptr, Role role = Role::Ring)
    {
        release();

        if (size == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0)
            return false;

        // Physical memory may live above 4 GiB. IODMACommand constrains the
        // device-visible address to 32 bits, as required by the legacy
        // QCA6174/QCA9377 CE descriptor format.
        const UInt64 physicalMask = 0xffffffffffffffffULL & ~(alignment - 1ULL);
        _descriptor = IOBufferMemoryDescriptor::inTaskWithPhysicalMask(
            kernel_task,
            kIOMemoryPhysicallyContiguous | kIODirectionInOut |
                (role == Role::Payload && ATH10K_PAYLOAD_CACHE ? kIOMapCopybackCache : kIOMapInhibitCache),
            size,
            physicalMask);

        if (!_descriptor)
            return false;

        if (_descriptor->prepare() != kIOReturnSuccess) {
            _descriptor->release();
            _descriptor = nullptr;
            return false;
        }
        _prepared = true;

        // A nullptr mapper is valid for IODMACommand. On systems where
        // IOMapper::copyMapperForDevice() returns nullptr, kMapped may still
        // satisfy the 32-bit restriction using the default mapping path and/or
        // internal double-buffering. 0.0.17 continues to allow this path with explicit synchronization.
        _command = IODMACommand::withSpecification(
            kIODMACommandOutputHost64,
            32,
            0,
            IODMACommand::kMapped,
            0,
            static_cast<UInt32>(alignment),
            mapper,
            nullptr);
        if (!_command) {
            release();
            return false;
        }

        if (_command->setMemoryDescriptor(_descriptor) != kIOReturnSuccess) {
            release();
            return false;
        }
        _commandHasDescriptor = true;

        IODMACommand::Segment64 segment{};
        UInt64 offset = 0;
        UInt32 numSegments = 1;
        if (_command->gen64IOVMSegments(&offset, &segment, &numSegments) != kIOReturnSuccess ||
            numSegments != 1) {
            release();
            return false;
        }

        _virtual = _descriptor->getBytesNoCopy();
        _physical = _descriptor->getPhysicalAddress();
        _dmaAddress = segment.fIOVMAddr;
        _segmentLength = segment.fLength;
        _length = size;
        _alignment = alignment;
        _explicitMapper = (mapper != nullptr);
        _cached = role == Role::Payload && ATH10K_PAYLOAD_CACHE;

        if (!_virtual || _dmaAddress == 0 || _dmaAddress > 0xffffffffULL ||
            _segmentLength < size || (_dmaAddress & (alignment - 1ULL)) != 0) {
            release();
            return false;
        }

        // setMemoryDescriptor() auto-prepares the DMA command. If the command
        // had to create an internal bounce/double buffer, that prepare may have
        // copied the pre-zero contents. Zero the CPU-visible memory and then
        // explicitly synchronize it to the device-visible DMA buffer.
        bzero(_virtual, static_cast<size_t>(_length));
        OSSynchronizeIO();
        if (!syncForDevice()) {
            release();
            return false;
        }
        return true;
    }

    bool syncForDevice()
    {
        if (!_command || !_commandHasDescriptor)
            return false;
        const IOReturn ret = _command->synchronize(kIODirectionOut);
        _lastSyncOut = ret;
        if (ret == kIOReturnSuccess)
            ++_syncOutCount;
        return ret == kIOReturnSuccess;
    }

    bool syncFromDevice()
    {
        if (!_command || !_commandHasDescriptor)
            return false;
        const IOReturn ret = _command->synchronize(kIODirectionIn);
        _lastSyncIn = ret;
        if (ret == kIOReturnSuccess)
            ++_syncInCount;
        return ret == kIOReturnSuccess;
    }

    void release()
    {
        if (_command) {
            if (_commandHasDescriptor)
                _command->clearMemoryDescriptor();
            _command->release();
        }
        _command = nullptr;
        _commandHasDescriptor = false;

        if (_descriptor) {
            if (_prepared)
                _descriptor->complete();
            _descriptor->release();
        }
        _descriptor = nullptr;
        _prepared = false;

        _virtual = nullptr;
        _physical = 0;
        _dmaAddress = 0;
        _segmentLength = 0;
        _length = 0;
        _alignment = 0;
        _explicitMapper = false;
        _cached = false;
        _syncOutCount = 0;
        _syncInCount = 0;
        _lastSyncOut = kIOReturnNotReady;
        _lastSyncIn = kIOReturnNotReady;
    }

    bool allocated() const
    {
        return _descriptor != nullptr && _command != nullptr && _virtual != nullptr;
    }

    void *virtualAddress() const { return _virtual; }
    IOPhysicalAddress physicalAddress() const { return _physical; }
    UInt64 dmaAddress() const { return _dmaAddress; }
    UInt64 segmentLength() const { return _segmentLength; }
    UInt64 length() const { return _length; }
    UInt64 alignment() const { return _alignment; }
    bool usesExplicitMapper() const { return _explicitMapper; }
    bool usesCache() const { return _cached; }
    const char *mapperModeName() const { return _explicitMapper ? "device-bound" : "default/null"; }
    UInt32 syncOutCount() const { return _syncOutCount; }
    UInt32 syncInCount() const { return _syncInCount; }
    IOReturn lastSyncOut() const { return _lastSyncOut; }
    IOReturn lastSyncIn() const { return _lastSyncIn; }

private:
    IOBufferMemoryDescriptor *_descriptor = nullptr;
    IODMACommand *_command = nullptr;
    bool _prepared = false;
    bool _commandHasDescriptor = false;

    void *_virtual = nullptr;
    IOPhysicalAddress _physical = 0;
    UInt64 _dmaAddress = 0;
    UInt64 _segmentLength = 0;
    UInt64 _length = 0;
    UInt64 _alignment = 0;
    bool _explicitMapper = false;
    bool _cached = false;
    UInt32 _syncOutCount = 0;
    UInt32 _syncInCount = 0;
    IOReturn _lastSyncOut = kIOReturnNotReady;
    IOReturn _lastSyncIn = kIOReturnNotReady;
};
