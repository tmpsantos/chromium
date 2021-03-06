// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "athena/extensions/public/extensions_delegate.h"

#include "athena/extensions/chrome/athena_apps_client.h"
#include "base/macros.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/extensions/application_launch.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_set.h"
#include "extensions/common/extension_urls.h"
#include "net/base/url_util.h"

namespace athena {
namespace {

class ChromeExtensionsDelegate : public ExtensionsDelegate {
 public:
  explicit ChromeExtensionsDelegate(content::BrowserContext* context)
      : extension_service_(
            extensions::ExtensionSystem::Get(context)->extension_service()) {
    extensions::AppsClient::Set(&apps_client_);
  }

  virtual ~ChromeExtensionsDelegate() {}

 private:
  // ExtensionsDelegate:
  virtual content::BrowserContext* GetBrowserContext() const OVERRIDE {
    return extension_service_->GetBrowserContext();
  }
  virtual const extensions::ExtensionSet& GetInstalledExtensions() OVERRIDE {
    return *extension_service_->extensions();
  }
  virtual bool LaunchApp(const std::string& app_id) OVERRIDE {
    // Check Running apps
    content::BrowserContext* context = GetBrowserContext();
    const extensions::Extension* extension =
        extensions::ExtensionRegistry::Get(context)->GetExtensionById(
            app_id, extensions::ExtensionRegistry::EVERYTHING);
    DCHECK(extension);
    if (!extension)
      return false;

    // TODO(oshima): Support installation/enabling process.
    if (!extensions::util::IsAppLaunchableWithoutEnabling(app_id, context))
      return false;

    int event_flags = 0;
    AppLaunchParams params(Profile::FromBrowserContext(context),
                           extension,
                           event_flags,
                           chrome::HOST_DESKTOP_TYPE_ASH);
    // TODO(oshima): rename HOST_DESTOP_TYPE_ASH to non native desktop.

    if (app_id == extensions::kWebStoreAppId) {
      std::string source_value =
          std::string(extension_urls::kLaunchSourceAppList);
      // Set an override URL to include the source.
      GURL extension_url =
          extensions::AppLaunchInfo::GetFullLaunchURL(extension);
      params.override_url = net::AppendQueryParameter(
          extension_url, extension_urls::kWebstoreSourceField, source_value);
    }
    params.container = extensions::LAUNCH_CONTAINER_WINDOW;

    OpenApplication(params);
    return true;
  }
  virtual bool UnloadApp(const std::string& app_id) OVERRIDE {
    // TODO(skuhne): Implement using extension service.
    return false;
  }

  // ExtensionService for the browser context this is created for.
  ExtensionService* extension_service_;

  // Installed extensions.
  extensions::ExtensionSet extensions_;

  AthenaAppsClient apps_client_;

  DISALLOW_COPY_AND_ASSIGN(ChromeExtensionsDelegate);
};

}  // namespace

// static
void ExtensionsDelegate::CreateExtensionsDelegateForChrome(
    content::BrowserContext* context) {
  new ChromeExtensionsDelegate(context);
}

}  // namespace athena
