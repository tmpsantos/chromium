// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_APPS_CHROME_APPS_CLIENT_H_
#define CHROME_BROWSER_UI_APPS_CHROME_APPS_CLIENT_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "extensions/browser/app_window/apps_client.h"

template <typename T>
struct DefaultSingletonTraits;

// The implementation of AppsClient for Chrome.
class ChromeAppsClient : public extensions::AppsClient {
 public:
  ChromeAppsClient();
  virtual ~ChromeAppsClient();

  // Get the LazyInstance for ChromeAppsClient.
  static ChromeAppsClient* GetInstance();

 private:
  friend struct DefaultSingletonTraits<ChromeAppsClient>;

  // extensions::AppsClient
  virtual std::vector<content::BrowserContext*> GetLoadedBrowserContexts()
      OVERRIDE;
  virtual extensions::AppWindow* CreateAppWindow(
      content::BrowserContext* context,
      const extensions::Extension* extension) OVERRIDE;
  virtual extensions::NativeAppWindow* CreateNativeAppWindow(
      extensions::AppWindow* window,
      const extensions::AppWindow::CreateParams& params) OVERRIDE;
  virtual void IncrementKeepAliveCount() OVERRIDE;
  virtual void DecrementKeepAliveCount() OVERRIDE;
  virtual void OpenDevToolsWindow(content::WebContents* web_contents,
                                  const base::Closure& callback) OVERRIDE;
  virtual bool IsCurrentChannelOlderThanDev() OVERRIDE;

  // Implemented in platform specific code.
  static extensions::NativeAppWindow* CreateNativeAppWindowImpl(
      extensions::AppWindow* window,
      const extensions::AppWindow::CreateParams& params);

  DISALLOW_COPY_AND_ASSIGN(ChromeAppsClient);
};

#endif  // CHROME_BROWSER_UI_APPS_CHROME_APPS_CLIENT_H_
