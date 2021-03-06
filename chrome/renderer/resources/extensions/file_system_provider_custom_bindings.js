// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Custom binding for the fileSystemProvider API.

var binding = require('binding').Binding.create('fileSystemProvider');
var fileSystemProviderInternal =
    require('binding').Binding.create('fileSystemProviderInternal').generate();
var eventBindings = require('event_bindings');
var fileSystemNatives = requireNative('file_system_natives');
var GetDOMError = fileSystemNatives.GetDOMError;

/**
 * Maximum size of the thumbnail in bytes.
 * @type {number}
 * @const
 */
var METADATA_THUMBNAIL_SIZE_LIMIT = 32 * 1024 * 1024;

/**
 * Regular expression to validate if the thumbnail URI is a valid data URI,
 * taking into account allowed formats.
 * @type {RegExp}
 * @const
 */
var METADATA_THUMBNAIL_FORMAT = new RegExp(
    '^data:image/(png|jpeg|webp);', 'i');

/**
 * Annotates a date with its serialized value.
 * @param {Date} date Input date.
 * @return {Date} Date with an extra <code>value</code> attribute.
 */
function annotateDate(date) {
  // Copy in case the input date is frozen.
  var result = new Date(date.getTime());
  result.value = result.toString();
  return result;
}

/**
 * Verifies if the passed image URI is valid.
 * @param {*} uri Image URI.
 * @return {boolean} True if valid, valse otherwise.
 */
function verifyImageURI(uri) {
  // The URI is specified by a user, so the type may be incorrect.
  if (typeof uri != 'string' && !(uri instanceof String))
    return false;

  return METADATA_THUMBNAIL_FORMAT.test(uri);
}

/**
 * Annotates an entry metadata by serializing its modifiedTime value.
 * @param {EntryMetadata} metadata Input metadata.
 * @return {EntryMetadata} metadata Annotated metadata, which can be passed
 *     back to the C++ layer.
 */
function annotateMetadata(metadata) {
  var result = {
    isDirectory: metadata.isDirectory,
    name: metadata.name,
    size: metadata.size,
    modificationTime: annotateDate(metadata.modificationTime)
  };
  if ('mimeType' in metadata)
    result.mimeType = metadata.mimeType;
  if ('thumbnail' in metadata)
    result.thumbnail = metadata.thumbnail;
  return result;
}

/**
 * Massages arguments of an event raised by the File System Provider API.
 * @param {Array.<*>} args Input arguments.
 * @param {function(Array.<*>)} dispatch Closure to be called with massaged
 *     arguments.
 */
function massageArgumentsDefault(args, dispatch) {
  var executionStart = Date.now();
  var options = args[0];
  var onSuccessCallback = function(hasNext) {
    fileSystemProviderInternal.operationRequestedSuccess(
        options.fileSystemId, options.requestId, Date.now() - executionStart);
  };
  var onErrorCallback = function(error) {
    fileSystemProviderInternal.operationRequestedError(
        options.fileSystemId, options.requestId, error,
        Date.now() - executionStart);
  }
  dispatch([options, onSuccessCallback, onErrorCallback]);
}


binding.registerCustomHook(function(bindingsAPI) {
  var apiFunctions = bindingsAPI.apiFunctions;

  apiFunctions.setUpdateArgumentsPostValidate(
    'mount',
    function(options, successCallback, errorCallback) {
      // Piggyback the error callback onto the success callback,
      // so we can use the error callback later.
      successCallback.errorCallback_ = errorCallback;
      return [options, successCallback];
    });

  apiFunctions.setCustomCallback(
    'mount',
    function(name, request, response) {
      var domError = null;
      if (request.callback && response) {
        // DOMError is present only if mount() failed.
        if (response[0]) {
          // Convert a Dictionary to a DOMError.
          domError = GetDOMError(response[0].name, response[0].message);
          response.length = 1;
        }

        var successCallback = request.callback;
        var errorCallback = request.callback.errorCallback_;
        delete request.callback;

        if (domError)
          errorCallback(domError);
        else
          successCallback();
      }
    });

  apiFunctions.setUpdateArgumentsPostValidate(
    'unmount',
    function(options, successCallback, errorCallback) {
      // Piggyback the error callback onto the success callback,
      // so we can use the error callback later.
      successCallback.errorCallback_ = errorCallback;
      return [options, successCallback];
    });

  apiFunctions.setCustomCallback(
    'unmount',
    function(name, request, response) {
      var domError = null;
      if (request.callback) {
        // DOMError is present only if mount() failed.
        if (response && response[0]) {
          // Convert a Dictionary to a DOMError.
          domError = GetDOMError(response[0].name, response[0].message);
          response.length = 1;
        }

        var successCallback = request.callback;
        var errorCallback = request.callback.errorCallback_;
        delete request.callback;

        if (domError)
          errorCallback(domError);
        else
          successCallback();
      }
    });
});

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onUnmountRequested',
    massageArgumentsDefault);

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onGetMetadataRequested',
    function(args, dispatch) {
      var executionStart = Date.now();
      var options = args[0];
      var onSuccessCallback = function(metadata) {
        // It is invalid to return a thumbnail when it's not requested. The
        // restriction is added in order to avoid fetching the thumbnail while
        // it's not needed.
        if (!options.thumbnail && metadata.thumbnail) {
          fileSystemProviderInternal.operationRequestedError(
              options.fileSystemId, options.requestId, 'FAILED',
              Date.now() - executionStart);
          throw new Error('Thumbnail data provided, but not requested.');
        }

        // Check the format and size. Note, that in the C++ layer, there is
        // another sanity check to avoid passing any evil URL.
        if ('thumbnail' in metadata && !verifyImageURI(metadata.thumbnail))
          throw new Error('Thumbnail format invalid.');
        if ('thumbnail' in metadata &&
            metadata.thumbnail.length > METADATA_THUMBNAIL_SIZE_LIMIT) {
          throw new Error('Thumbnail data too large.');
        }

        fileSystemProviderInternal.getMetadataRequestedSuccess(
            options.fileSystemId,
            options.requestId,
            annotateMetadata(metadata),
            Date.now() - executionStart);
      };
      var onErrorCallback = function(error) {
        fileSystemProviderInternal.operationRequestedError(
            options.fileSystemId, options.requestId, error,
            Date.now() - executionStart);
      }
      dispatch([options, onSuccessCallback, onErrorCallback]);
    });

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onReadDirectoryRequested',
    function(args, dispatch) {
      var executionStart = Date.now();
      var options = args[0];
      var onSuccessCallback = function(entries, hasNext) {
        var annotatedEntries = entries.map(annotateMetadata);
        // It is invalid to return a thumbnail when it's not requested.
        annotatedEntries.forEach(function(metadata) {
          if (metadata.thumbnail) {
            fileSystemProviderInternal.operationRequestedError(
                options.fileSystemId, options.requestId, 'FAILED',
                Date.now() - executionStart);
            throw new Error(
                'Thumbnails must not be provided when reading a directory.');
          }
        });
        fileSystemProviderInternal.readDirectoryRequestedSuccess(
            options.fileSystemId, options.requestId, annotatedEntries, hasNext,
            Date.now() - executionStart);
      };
      var onErrorCallback = function(error) {
        fileSystemProviderInternal.operationRequestedError(
            options.fileSystemId, options.requestId, error,
            Date.now() - executionStart);
      }
      dispatch([options, onSuccessCallback, onErrorCallback]);
    });

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onOpenFileRequested',
    massageArgumentsDefault);

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onCloseFileRequested',
    massageArgumentsDefault);

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onReadFileRequested',
    function(args, dispatch) {
      var executionStart = Date.now();
      var options = args[0];
      var onSuccessCallback = function(data, hasNext) {
        fileSystemProviderInternal.readFileRequestedSuccess(
            options.fileSystemId, options.requestId, data, hasNext,
            Date.now() - executionStart);
      };
      var onErrorCallback = function(error) {
        fileSystemProviderInternal.operationRequestedError(
            options.fileSystemId, options.requestId, error,
            Date.now() - executionStart);
      }
      dispatch([options, onSuccessCallback, onErrorCallback]);
    });

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onCreateDirectoryRequested',
    massageArgumentsDefault);

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onDeleteEntryRequested',
    massageArgumentsDefault);

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onCreateFileRequested',
    massageArgumentsDefault);

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onCopyEntryRequested',
    massageArgumentsDefault);

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onMoveEntryRequested',
    massageArgumentsDefault);

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onTruncateRequested',
    massageArgumentsDefault);

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onWriteFileRequested',
    massageArgumentsDefault);

eventBindings.registerArgumentMassager(
    'fileSystemProvider.onAbortRequested',
    massageArgumentsDefault);

exports.binding = binding.generate();
