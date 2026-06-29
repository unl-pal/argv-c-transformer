#pragma once

#include <clang/AST/Type.h>
#include <optional>
#include <string>
#include <unordered_map>

/**
 * @file VerifierNames.hpp
 * @brief Single source of truth for SV-Comp __VERIFIER_nondet_* naming.
 *
 */

/**
 * @brief Canonical mapping from Clang builtin type kind to SV-Comp verifier suffix.
 *
 * Types not in this map are unsupported (e.g. pointers, structs) and callers
 * should skip them.
 */
inline const std::unordered_map<clang::BuiltinType::Kind, std::string> kVerifierNames = {
    {clang::BuiltinType::Bool, "bool"},           {clang::BuiltinType::Char_S, "char"},
    {clang::BuiltinType::Char_U, "char"},         {clang::BuiltinType::SChar, "char"},
    {clang::BuiltinType::UChar, "uchar"},         {clang::BuiltinType::Short, "short"},
    {clang::BuiltinType::UShort, "ushort"},       {clang::BuiltinType::Int, "int"},
    {clang::BuiltinType::UInt, "uint"},           {clang::BuiltinType::Long, "long"},
    {clang::BuiltinType::ULong, "ulong"},         {clang::BuiltinType::LongLong, "longlong"},
    {clang::BuiltinType::ULongLong, "ulonglong"}, {clang::BuiltinType::Float, "float"},
    {clang::BuiltinType::Double, "double"},
};

/**
 * @brief Maps verifier suffix to the C type spelling used in source-level
 * declarations (e.g. "uint" -> "unsigned int").
 */
inline const std::unordered_map<std::string, std::string> kVerifierCTypes = {
    {"bool", "_Bool"},
    {"char", "char"},
    {"uchar", "unsigned char"},
    {"short", "short"},
    {"ushort", "unsigned short"},
    {"int", "int"},
    {"uint", "unsigned int"},
    {"long", "long"},
    {"ulong", "unsigned long"},
    {"longlong", "long long"},
    {"ulonglong", "unsigned long long"},
    {"float", "float"},
    {"double", "double"},
};

/**
 * @brief Returns the C type spelling for a verifier suffix.
 *
 * @param suffix Verifier suffix (e.g. "uint").
 * @return The C type spelling (e.g. "unsigned int"), or std::nullopt if unknown.
 */
inline std::optional<std::string> cTypeForSuffix(const std::string &suffix) {
  auto it = kVerifierCTypes.find(suffix);
  if (it == kVerifierCTypes.end())
    return std::nullopt;
  return it->second;
}

/**
 * @brief Returns the verifier suffix for a type.
 *
 * @param QT The type to resolve (e.g. `unsigned int`).
 * @return The suffix (e.g. "uint"), or std::nullopt if not a supported builtin.
 */
inline std::optional<std::string> verifierSuffixForType(clang::QualType QT) {
  // A null type comes back from error-recovery AST nodes (e.g. calls or
  // params built from undefined macros when headers are missing).
  if (QT.isNull())
    return std::nullopt;
  const clang::BuiltinType *BT = QT->getAs<clang::BuiltinType>();
  if (!BT)
    return std::nullopt;
  auto it = kVerifierNames.find(BT->getKind());
  if (it == kVerifierNames.end())
    return std::nullopt;
  return it->second;
}

/**
 * @brief Returns the full verifier function name for a type.
 *
 * @param QT The type to resolve.
 * @return The name (e.g. "__VERIFIER_nondet_uint"), or std::nullopt if unsupported.
 */
inline std::optional<std::string> verifierFnNameForType(clang::QualType QT) {
  std::optional<std::string> suffix = verifierSuffixForType(QT);
  if (!suffix)
    return std::nullopt;
  return "__VERIFIER_nondet_" + *suffix;
}
