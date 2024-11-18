#include "wrapper.h"
#include "llvm/IR/Module.h"
#include "LLVMSPIRVLib.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <cstring>

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

std::unique_ptr<Module> parseSpirvToModule(const std::vector<uint8_t>& spirvBuffer, std::string& errors) {
    LLVMContext context;
    std::istringstream spirvStream(std::string(spirvBuffer.begin(), spirvBuffer.end()));
    std::unique_ptr<Module> module;

    if (!readSpirv(context, spirvStream, module, errors)) {
        return nullptr;
    }

    return module;
}

std::string generateBitcodeFromModule(std::unique_ptr<Module>& module) {
    std::string bitcode;
    raw_string_ostream bitcodeStream(bitcode);
    bitcodeStream << *module;
    bitcodeStream.flush();
    return bitcode;
}

extern "C" ITranslator_Result* generateBitcode(const char* spirvBuffer, size_t spirvSize) {
    if (!spirvBuffer || spirvSize == 0) {
        return new ITranslator_Result("", "Invalid input: SPIR-V buffer is null or size is zero.");
    }

    if (!IsValidSpirv(spirvBuffer, spirvSize)) {
        return new ITranslator_Result("", "Invalid SPIR-V buffer: does not match SPIR-V magic number.");
    }

    std::vector<uint8_t> spirvBufferVector(spirvBuffer, spirvBuffer + spirvSize);
    
    std::string errors;
    auto module = parseSpirvToModule(spirvBufferVector, errors);
    
    if (!module) {
        return new ITranslator_Result("", errors.empty() ? "Failed to parse SPIR-V." : errors);
    }

    std::string bitcode = generateBitcodeFromModule(module);

    return new ITranslator_Result(std::move(bitcode), std::move(errors));
}
