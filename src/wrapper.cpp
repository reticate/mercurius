#include "wrapper.h"

#include "llvm/IR/Module.h"
#include "LLVMSPIRVLib.h"

#include <cstdint>
#include <string>
#include <memory>

using namespace llvm;

namespace {

bool isSpirvMagicNumberValid(const char *buffer, size_t size) {
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

}

extern "C" std::unique_ptr<ITranslator_Result> generateBitcode(const char *spirvBuffer, size_t spirvSize) {
    if (!spirvBuffer || spirvSize < 4) {
        return std::make_unique<ITranslator_Result>("", "Invalid input: SPIR-V buffer is null or size is too small.");
    }

    if (!isSpirvMagicNumberValid(spirvBuffer, spirvSize)) {
        return std::make_unique<ITranslator_Result>("", "Invalid SPIR-V buffer: does not match SPIR-V magic number.");
    }

    StringRef spirvRef(spirvBuffer, spirvSize);
    raw_string_ostream errorsStream(errors);

    LLVMContext context;
    std::unique_ptr<Module> module;

    if (!readSpirv(context, spirvRef, module, errors)) {
        return std::make_unique<ITranslator_Result>("", errorsStream.str());
    }

    if (!module) {
        return std::make_unique<ITranslator_Result>("", "Failed to create LLVM module from SPIR-V.");
    }

    std::string bitcode;
    raw_string_ostream bitcodeStream(bitcode);
    bitcodeStream << *module;

    if (!bitcode.empty()) {
        bitcodeStream.flush();
    }

    return std::make_unique<ITranslator_Result>(std::move(bitcode), std::move(errorsStream.str()));
}
