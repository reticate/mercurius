#pragma once

#include <string>
#include <memory>
#include <vector>

struct ITranslator_Result {
    ITranslator_Result(std::string resultBitcode, std::string errors)
        : resultBitcode(std::move(resultBitcode)), errors(std::move(errors)) {}

    virtual const char* GetBitcode() const { return resultBitcode.empty() ? nullptr : resultBitcode.data(); }
    virtual size_t GetBitcodeSize() const { return resultBitcode.size(); }
    virtual const char* GetErrorLog() const { return errors.empty() ? nullptr : errors.c_str(); }
    virtual void Release() { delete this; }
    virtual bool HasErrors() const { return !errors.empty(); }
    virtual bool IsEmpty() const { return resultBitcode.empty(); }

protected:
    virtual ~ITranslator_Result() = default;

private:
    std::string resultBitcode;
    std::string errors;
};

extern "C" ITranslator_Result* generateBitcode(const char* spirvBuffer, size_t spirvSize);
bool IsValidSpirv(const char* buffer, size_t size);

std::unique_ptr<llvm::Module> parseSpirvToModule(const std::vector<uint8_t>& spirvBuffer, std::string& errors);
std::string generateBitcodeFromModule(std::unique_ptr<llvm::Module>& module);
