// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media_galleries/fileapi/picasa_data_provider.h"

#include <utility>

#include "base/basictypes.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/media_galleries/fileapi/file_path_watcher_util.h"
#include "chrome/browser/media_galleries/fileapi/media_file_system_backend.h"
#include "chrome/browser/media_galleries/fileapi/safe_picasa_album_table_reader.h"
#include "chrome/browser/media_galleries/fileapi/safe_picasa_albums_indexer.h"
#include "chrome/browser/media_galleries/imported_media_gallery_registry.h"
#include "webkit/browser/fileapi/file_system_operation_context.h"
#include "webkit/browser/fileapi/file_system_url.h"

namespace picasa {

namespace {

void RunAllCallbacks(
    std::vector<PicasaDataProvider::ReadyCallback>* ready_callbacks,
    bool success) {
  for (std::vector<PicasaDataProvider::ReadyCallback>::const_iterator it =
           ready_callbacks->begin();
       it != ready_callbacks->end();
       ++it) {
    it->Run(success);
  }
  ready_callbacks->clear();
}

}  // namespace

PicasaDataProvider::PicasaDataProvider(const base::FilePath& database_path)
    : database_path_(database_path),
      state_(STALE_DATA_STATE),
      weak_factory_(this) {
  DCHECK(MediaFileSystemBackend::CurrentlyOnMediaTaskRunnerThread());

  StartFilePathWatchOnMediaTaskRunner(
      database_path_.DirName().AppendASCII(kPicasaTempDirName),
      base::Bind(&PicasaDataProvider::OnTempDirWatchStarted,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&PicasaDataProvider::OnTempDirChanged,
                 weak_factory_.GetWeakPtr()));
}

PicasaDataProvider::~PicasaDataProvider() {}

void PicasaDataProvider::RefreshData(DataType needed_data,
                                     const ReadyCallback& ready_callback) {
  DCHECK(MediaFileSystemBackend::CurrentlyOnMediaTaskRunnerThread());
  // TODO(tommycli): Need to watch the database_path_ folder and handle
  // rereading the data when it changes.

  if (state_ == INVALID_DATA_STATE) {
    ready_callback.Run(false /* success */);
    return;
  }

  if (needed_data == LIST_OF_ALBUMS_AND_FOLDERS_DATA) {
    if (state_ == LIST_OF_ALBUMS_AND_FOLDERS_FRESH_STATE ||
        state_ == ALBUMS_IMAGES_FRESH_STATE) {
      ready_callback.Run(true /* success */);
      return;
    }
    album_list_ready_callbacks_.push_back(ready_callback);
  } else {
    if (state_ == ALBUMS_IMAGES_FRESH_STATE) {
      ready_callback.Run(true /* success */);
      return;
    }
    albums_index_ready_callbacks_.push_back(ready_callback);
  }
  DoRefreshIfNecessary();
}

scoped_ptr<AlbumMap> PicasaDataProvider::GetFolders() {
  DCHECK(MediaFileSystemBackend::CurrentlyOnMediaTaskRunnerThread());
  DCHECK(state_ == LIST_OF_ALBUMS_AND_FOLDERS_FRESH_STATE ||
         state_ == ALBUMS_IMAGES_FRESH_STATE);
  return make_scoped_ptr(new AlbumMap(folder_map_));
}

scoped_ptr<AlbumMap> PicasaDataProvider::GetAlbums() {
  DCHECK(MediaFileSystemBackend::CurrentlyOnMediaTaskRunnerThread());
  DCHECK(state_ == LIST_OF_ALBUMS_AND_FOLDERS_FRESH_STATE ||
         state_ == ALBUMS_IMAGES_FRESH_STATE);
  return make_scoped_ptr(new AlbumMap(album_map_));
}

scoped_ptr<AlbumImages> PicasaDataProvider::FindAlbumImages(
    const std::string& key,
    base::File::Error* error) {
  DCHECK(MediaFileSystemBackend::CurrentlyOnMediaTaskRunnerThread());
  DCHECK(state_ == ALBUMS_IMAGES_FRESH_STATE);
  DCHECK(error);

  AlbumImagesMap::const_iterator it = albums_images_.find(key);

  if (it == albums_images_.end()) {
    *error = base::File::FILE_ERROR_NOT_FOUND;
    return scoped_ptr<AlbumImages>();
  }

  *error = base::File::FILE_OK;
  return make_scoped_ptr(new AlbumImages(it->second));
}

void PicasaDataProvider::InvalidateData() {
  DCHECK(MediaFileSystemBackend::CurrentlyOnMediaTaskRunnerThread());

  // Set data state to stale and ignore responses from any in-flight processes.
  // TODO(tommycli): Implement and call Cancel function for these
  // UtilityProcessHostClients to actually kill the in-flight processes.
  state_ = STALE_DATA_STATE;
  album_table_reader_ = NULL;
  albums_indexer_ = NULL;

  DoRefreshIfNecessary();
}

void PicasaDataProvider::OnTempDirWatchStarted(
    scoped_ptr<base::FilePathWatcher> temp_dir_watcher) {
  DCHECK(MediaFileSystemBackend::CurrentlyOnMediaTaskRunnerThread());
  temp_dir_watcher_.reset(temp_dir_watcher.release());
}

void PicasaDataProvider::OnTempDirChanged(const base::FilePath& temp_dir_path,
                                          bool error) {
  DCHECK(MediaFileSystemBackend::CurrentlyOnMediaTaskRunnerThread());
  if (base::IsDirectoryEmpty(temp_dir_path))
    InvalidateData();
}

void PicasaDataProvider::DoRefreshIfNecessary() {
  DCHECK(state_ != INVALID_DATA_STATE);
  DCHECK(state_ != ALBUMS_IMAGES_FRESH_STATE);
  DCHECK(!(album_table_reader_ && albums_indexer_));

  if (album_list_ready_callbacks_.empty() &&
      albums_index_ready_callbacks_.empty()) {
    return;
  }

  if (state_ == STALE_DATA_STATE) {
    if (album_table_reader_)
      return;
    album_table_reader_ =
        new SafePicasaAlbumTableReader(AlbumTableFiles(database_path_));
    album_table_reader_->Start(
        base::Bind(&PicasaDataProvider::OnAlbumTableReaderDone,
                   weak_factory_.GetWeakPtr(),
                   album_table_reader_));
  } else {
    DCHECK(state_ == LIST_OF_ALBUMS_AND_FOLDERS_FRESH_STATE);
    if (albums_indexer_)
      return;
    albums_indexer_ = new SafePicasaAlbumsIndexer(album_map_, folder_map_);
    albums_indexer_->Start(base::Bind(&PicasaDataProvider::OnAlbumsIndexerDone,
                                      weak_factory_.GetWeakPtr(),
                                      albums_indexer_));
  }
}

void PicasaDataProvider::OnAlbumTableReaderDone(
    scoped_refptr<SafePicasaAlbumTableReader> reader,
    bool parse_success,
    const std::vector<AlbumInfo>& albums,
    const std::vector<AlbumInfo>& folders) {
  DCHECK(MediaFileSystemBackend::CurrentlyOnMediaTaskRunnerThread());
  // If the reader has already been deemed stale, ignore the result.
  if (reader != album_table_reader_)
    return;
  album_table_reader_ = NULL;

  DCHECK(state_ == STALE_DATA_STATE);

  if (!parse_success) {
    // If we didn't get the list successfully, fail all those waiting for
    // the albums indexer also.
    state_ = INVALID_DATA_STATE;
    RunAllCallbacks(&album_list_ready_callbacks_, false /* success */);
    RunAllCallbacks(&albums_index_ready_callbacks_, false /* success */);
    return;
  }

  album_map_.clear();
  folder_map_.clear();
  UniquifyNames(albums, &album_map_);
  UniquifyNames(folders, &folder_map_);

  state_ = LIST_OF_ALBUMS_AND_FOLDERS_FRESH_STATE;
  RunAllCallbacks(&album_list_ready_callbacks_, parse_success);

  // Chain from this process onto refreshing the albums images if necessary.
  DoRefreshIfNecessary();
}

void PicasaDataProvider::OnAlbumsIndexerDone(
    scoped_refptr<SafePicasaAlbumsIndexer> indexer,
    bool success,
    const picasa::AlbumImagesMap& albums_images) {
  DCHECK(MediaFileSystemBackend::CurrentlyOnMediaTaskRunnerThread());
  // If the indexer has already been deemed stale, ignore the result.
  if (indexer != albums_indexer_)
    return;
  albums_indexer_ = NULL;

  DCHECK(state_ == LIST_OF_ALBUMS_AND_FOLDERS_FRESH_STATE);

  if (success) {
    state_ = ALBUMS_IMAGES_FRESH_STATE;

    albums_images_ = albums_images;
  }

  RunAllCallbacks(&albums_index_ready_callbacks_, success);
}

// static
std::string PicasaDataProvider::DateToPathString(const base::Time& time) {
  base::Time::Exploded exploded_time;
  time.LocalExplode(&exploded_time);

  // TODO(tommycli): Investigate better localization and persisting which locale
  // we use to generate these unique names.
  return base::StringPrintf("%04d-%02d-%02d", exploded_time.year,
                            exploded_time.month, exploded_time.day_of_month);
}

// static
void PicasaDataProvider::UniquifyNames(const std::vector<AlbumInfo>& info_list,
                                       AlbumMap* result_map) {
  // TODO(tommycli): We should persist the uniquified names.
  std::vector<std::string> desired_names;

  std::map<std::string, int> total_counts;
  std::map<std::string, int> current_counts;

  for (std::vector<AlbumInfo>::const_iterator it = info_list.begin();
       it != info_list.end(); ++it) {
    std::string desired_name =
        it->name + " " + DateToPathString(it->timestamp);
    desired_names.push_back(desired_name);
    ++total_counts[desired_name];
  }

  for (unsigned int i = 0; i < info_list.size(); i++) {
    std::string name = desired_names[i];

    if (total_counts[name] != 1) {
      name = base::StringPrintf("%s (%d)", name.c_str(),
                                ++current_counts[name]);
    }

    result_map->insert(std::pair<std::string, AlbumInfo>(name, info_list[i]));
  }
}

}  // namespace picasa
