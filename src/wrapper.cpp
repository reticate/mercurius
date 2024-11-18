#include "wrapper.h"

#include "llvm/IR/Module.h"
#include "LLVMSPIRVLib.h"

#include <cstdint>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <future>
#include <functional>

using namespace llvm;

bool IsValidSpirv(const char *buffer, size_t size) {
    if (buffer == nullptr || size < 4) {
        return false;
    }

    constexpr uint32_t spirvMagicNumber = 0x07230203;
    uint32_t magicNumber = *reinterpret_cast<const uint32_t *>(buffer);

    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    magicNumber = __builtin_bswap32(magicNumber);
    #endif

    return magicNumber == spirvMagicNumber;
}

std::string ProcessSpirvChunk(const char *chunk, size_t chunkSize, std::string &errors, LLVMContext &context) {
    std::istringstream spirvStream(std::string(chunk, chunkSize));
    std::unique_ptr<Module> module;

    if (!readSpirv(context, spirvStream, module, errors)) {
        return "";
    }

    std::string bitcode;
    if (module) {
        raw_string_ostream bitcodeStream(bitcode);
        bitcodeStream << *module;
        bitcodeStream.flush();
    }

    return bitcode;
}

extern "C" ITranslator_Result *generateBitcode(const char *spirvBuffer, size_t spirvSize) {
    if (!spirvBuffer || spirvSize == 0) {
        return new ITranslator_Result("", "Invalid input: SPIR-V buffer is null or size is zero.");
    }

    if (!IsValidSpirv(spirvBuffer, spirvSize)) {
        return new ITranslator_Result("", "Invalid SPIR-V buffer: does not match SPIR-V magic number.");
    }

    const size_t chunkSize = 1024 * 1024; 
    size_t numChunks = (spirvSize + chunkSize - 1) / chunkSize;
    std::vector<std::future<std::string>> futures;
    std::vector<std::string> errors(numChunks);
    LLVMContext context;

    for (size_t i = 0; i < numChunks; ++i) {
        size_t offset = i * chunkSize;
        size_t size = std::min(chunkSize, spirvSize - offset);

        futures.push_back(std::async(std::launch::async, ProcessSpirvChunk, spirvBuffer + offset, size, std::ref(errors[i]), std::ref(context)));
    }

    std::string finalBitcode;
    std::string finalErrors;

    for (size_t i = 0; i < numChunks; ++i) {
        std::string result = futures[i].get();
        if (result.empty() && !errors[i].empty()) {
            finalErrors += errors[i] + "\n";
        } else {
            finalBitcode += result;
        }
    }

    if (!finalErrors.empty()) {
        return new ITranslator_Result("", finalErrors);
    }

    return new ITranslator_Result(std::move(finalBitcode), "");
}
