// ids_header.h
//
// Pure text serializer: turns a list of (ResID, namekey) pairs into a C header
// that exposes each ResID as a macro whose value is the precomputed hash. No
// Excel or IO dependency.
#pragma once

#include <string>
#include <vector>

#include "packer.h"

namespace strres {

// Render string_ids.h contents: a banner, `#pragma once`, and one
// `#define <ResID> 0x........u` per id, in the given order. Macro names are
// emitted verbatim; values are 8-digit lowercase unsigned hex.
std::string render_ids_header(const std::vector<StringId>& ids);

}  // namespace strres
