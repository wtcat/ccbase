// Vendored replacement for the file miniz's CMake build normally generates via
// generate_export_header(). We compile miniz as a static library, so every
// visibility macro expands to nothing.
#ifndef MINIZ_EXPORT_H
#define MINIZ_EXPORT_H

#define MINIZ_EXPORT
#define MINIZ_NO_EXPORT
#define MINIZ_DEPRECATED
#define MINIZ_DEPRECATED_EXPORT
#define MINIZ_DEPRECATED_NO_EXPORT

#endif  // MINIZ_EXPORT_H
