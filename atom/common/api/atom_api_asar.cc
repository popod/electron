// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <vector>

#include "atom/common/asar/archive.h"
#include "atom/common/asar/archive_factory.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "native_mate/wrappable.h"

#include "atom/common/node_includes.h"

namespace {

class Archive : public mate::Wrappable {
 public:
  static v8::Handle<v8::Value> Create(v8::Isolate* isolate,
                                      const base::FilePath& path) {
    static asar::ArchiveFactory archive_factory;
    asar::Archive* archive = archive_factory.GetOrCreate(path);
    if (!archive)
      return v8::False(isolate);
    return (new Archive(archive))->GetWrapper(isolate);
  }

 protected:
  explicit Archive(asar::Archive* archive) : archive_(archive) {}
  virtual ~Archive() {}

  // Reads the offset and size of file.
  v8::Handle<v8::Value> GetFileInfo(v8::Isolate* isolate,
                                    const base::FilePath& path) {
    asar::Archive::FileInfo info;
    if (!archive_ || !archive_->GetFileInfo(path, &info))
      return v8::False(isolate);
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("size", info.size);
    dict.Set("offset", info.offset);
    return dict.GetHandle();
  }

  // Returns a fake result of fs.stat(path).
  v8::Handle<v8::Value> Stat(v8::Isolate* isolate,
                             const base::FilePath& path) {
    asar::Archive::Stats stats;
    if (!archive_ || !archive_->Stat(path, &stats))
      return v8::False(isolate);
    mate::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("size", stats.size);
    dict.Set("offset", stats.offset);
    dict.Set("isFile", stats.is_file);
    dict.Set("isDirectory", stats.is_directory);
    dict.Set("isLink", stats.is_link);
    return dict.GetHandle();
  }

  // Returns all files under a directory.
  v8::Handle<v8::Value> Readdir(v8::Isolate* isolate,
                                const base::FilePath& path) {
    std::vector<base::FilePath> files;
    if (!archive_ || !archive_->Readdir(path, &files))
      return v8::False(isolate);
    return mate::ConvertToV8(isolate, files);
  }

  // Copy the file out into a temporary file and returns the new path.
  v8::Handle<v8::Value> CopyFileOut(v8::Isolate* isolate,
                                    const base::FilePath& path) {
    base::FilePath new_path;
    if (!archive_ || !archive_->CopyFileOut(path, &new_path))
      return v8::False(isolate);
    return mate::ConvertToV8(isolate, new_path);
  }

  // mate::Wrappable:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(v8::Isolate* isolate) {
    return mate::ObjectTemplateBuilder(isolate)
        .SetValue("path", archive_->path())
        .SetMethod("getFileInfo", &Archive::GetFileInfo)
        .SetMethod("stat", &Archive::Stat)
        .SetMethod("readdir", &Archive::Readdir)
        .SetMethod("copyFileOut", &Archive::CopyFileOut);
  }

 private:
  asar::Archive* archive_;

  DISALLOW_COPY_AND_ASSIGN(Archive);
};

void Initialize(v8::Handle<v8::Object> exports, v8::Handle<v8::Value> unused,
                v8::Handle<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("createArchive", &Archive::Create);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_asar, Initialize)
