# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'athena_lib',
      'type': '<(component)',
      'dependencies': [
        # status_icon_container_view.cc depends on this. Remove this once there
        # are athena specific assets.
        '../ash/ash_resources.gyp:ash_resources',
        '../base/base.gyp:test_support_base',
        '../chromeos/chromeos.gyp:power_manager_proto',
        '../extensions/shell/app_shell.gyp:app_shell_version_header',
        '../ipc/ipc.gyp:ipc',
        '../skia/skia.gyp:skia',
        '../ui/accessibility/accessibility.gyp:ax_gen',
        '../ui/app_list/app_list.gyp:app_list',
        '../ui/aura/aura.gyp:aura',
        '../ui/aura/aura.gyp:aura_test_support',
        '../ui/chromeos/ui_chromeos.gyp:ui_chromeos',
        '../ui/display/display.gyp:display',
        '../ui/events/events.gyp:events_base',
        '../ui/strings/ui_strings.gyp:ui_strings',
        '../ui/views/views.gyp:views',
        'resources/athena_resources.gyp:athena_resources',
      ],
      'defines': [
        'ATHENA_IMPLEMENTATION',
      ],
      'sources': [
        # All .cc, .h under athena, except unittests
        'activity/activity.cc',
        'activity/activity_factory.cc',
        'activity/activity_manager_impl.cc',
        'activity/activity_manager_impl.h',
        'activity/activity_view_manager_impl.cc',
        'activity/activity_frame_view.cc',
        'activity/activity_frame_view.h',
        'activity/activity_widget_delegate.cc',
        'activity/activity_widget_delegate.h',
        'activity/public/activity.h',
        'activity/public/activity_factory.h',
        'activity/public/activity_manager.h',
        'activity/public/activity_manager_observer.h',
        'activity/public/activity_view_manager.h',
        'activity/public/activity_view_model.h',
        'athena_export.h',
        'common/container_priorities.h',
        'common/fill_layout_manager.cc',
        'common/fill_layout_manager.h',
        'common/switches.cc',
        'common/switches.h',
        'env/athena_env_impl.cc',
        'env/public/athena_env.h',
        'home/app_list_view_delegate.cc',
        'home/app_list_view_delegate.h',
        'home/athena_start_page_view.cc',
        'home/athena_start_page_view.h',
        'home/home_card_constants.cc',
        'home/home_card_constants.h',
        'home/home_card_gesture_manager.cc',
        'home/home_card_gesture_manager.h',
        'home/home_card_impl.cc',
        'home/minimized_home.cc',
        'home/minimized_home.h',
        'home/public/app_model_builder.h',
        'home/public/home_card.h',
        'input/accelerator_manager_impl.cc',
        'input/accelerator_manager_impl.h',
        'input/input_manager_impl.cc',
        'input/public/accelerator_manager.h',
        'input/public/input_manager.h',
        'resource_manager/delegate/resource_manager_delegate.cc',
        'resource_manager/memory_pressure_notifier.cc',
        'resource_manager/memory_pressure_notifier.h',
        'resource_manager/public/resource_manager.h',
        'resource_manager/public/resource_manager_delegate.h',
        'resource_manager/resource_manager_impl.cc',
        'screen/public/screen_manager.h',
        'screen/screen_accelerator_handler.cc',
        'screen/screen_accelerator_handler.h',
        'screen/screen_manager_impl.cc',
        'system/background_controller.cc',
        'system/background_controller.h',
        'system/device_socket_listener.cc',
        'system/device_socket_listener.h',
        'system/network_selector.cc',
        'system/network_selector.h',
        'system/orientation_controller.cc',
        'system/orientation_controller.h',
        'system/power_button_controller.cc',
        'system/power_button_controller.h',
        'system/status_icon_container_view.cc',
        'system/status_icon_container_view.h',
        'system/time_view.cc',
        'system/time_view.h',
        'system/public/system_ui.h',
        'system/system_ui_impl.cc',
        'wm/bezel_controller.cc',
        'wm/bezel_controller.h',
        'wm/overview_toolbar.cc',
        'wm/overview_toolbar.h',
        'wm/public/window_list_provider.h',
        'wm/public/window_manager.h',
        'wm/public/window_manager_observer.h',
        'wm/split_view_controller.cc',
        'wm/split_view_controller.h',
        'wm/title_drag_controller.cc',
        'wm/title_drag_controller.h',
        'wm/window_list_provider_impl.cc',
        'wm/window_list_provider_impl.h',
        'wm/window_manager_impl.cc',
        'wm/window_overview_mode.cc',
        'wm/window_overview_mode.h',
      ],
    },
    {
      'target_name': 'athena_content_lib',
      'type': 'static_library',
      'dependencies': [
        'athena_lib',
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../components/components.gyp:renderer_context_menu',
        '../components/components.gyp:web_modal',
        '../extensions/extensions.gyp:extensions_browser',
        '../extensions/extensions.gyp:extensions_common',
        '../content/content.gyp:content_browser',
        '../ui/app_list/app_list.gyp:app_list',
        '../ui/keyboard/keyboard.gyp:keyboard',
        '../ui/keyboard/keyboard.gyp:keyboard_resources',
        '../third_party/WebKit/public/blink.gyp:blink',
        '../ui/views/controls/webview/webview.gyp:webview',
        '../skia/skia.gyp:skia',
      ],
      'sources': [
        'content/app_activity.cc',
        'content/app_activity.h',
        'content/app_activity_proxy.cc',
        'content/app_activity_proxy.h',
        'content/app_activity_registry.cc',
        'content/app_activity_registry.h',
        'content/app_registry_impl.cc',
        'content/content_activity_factory.cc',
        'content/content_app_model_builder.cc',
        'content/public/app_registry.h',
        'content/content_activity_factory.h',
        'content/public/content_activity_factory_creator.h',
        'content/public/content_app_model_builder.h',
        'content/public/web_contents_view_delegate_creator.h',
        'content/render_view_context_menu_impl.cc',
        'content/render_view_context_menu_impl.h',
        'content/web_activity.cc',
        'content/web_activity.h',
        'content/web_contents_view_delegate_factory_impl.cc',
        'extensions/public/extensions_delegate.h',
        'extensions/extensions_delegate.cc',
        'virtual_keyboard/public/virtual_keyboard_manager.h',
        'virtual_keyboard/virtual_keyboard_manager_impl.cc',
      ],
    },
    {
      'target_name': 'athena_chrome_lib',
      'type': 'static_library',
      'dependencies': [
        '../chrome/chrome.gyp:browser_extensions',
      ],
      'sources': [
        'content/chrome/content_activity_factory.cc',
        'extensions/chrome/athena_apps_client.h',
        'extensions/chrome/athena_apps_client.cc',
        'extensions/chrome/extensions_delegate_impl.cc',
      ],
    },
    {
      'target_name': 'athena_app_shell_lib',
      'type': 'static_library',
      'dependencies': [
        '../extensions/shell/app_shell.gyp:app_shell_lib',
      ],
      'sources': [
        'content/shell/content_activity_factory.cc',
        'content/shell/shell_app_activity.cc',
        'content/shell/shell_app_activity.h',
        'extensions/shell/extensions_delegate_impl.cc',
      ],
    },
    {
      'target_name': 'athena_test_support',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:test_support_base',
        '../chromeos/chromeos.gyp:chromeos',
        '../skia/skia.gyp:skia',
        '../testing/gtest.gyp:gtest',
        '../ui/accessibility/accessibility.gyp:ax_gen',
        '../ui/app_list/app_list.gyp:app_list',
        '../ui/app_list/app_list.gyp:app_list_test_support',
        '../ui/aura/aura.gyp:aura_test_support',
        '../ui/base/ui_base.gyp:ui_base_test_support',
        '../ui/compositor/compositor.gyp:compositor_test_support',
        '../ui/views/views.gyp:views',
        '../ui/wm/wm.gyp:wm',
        '../url/url.gyp:url_lib',
        'athena_content_lib',
        'athena_lib',
        'resources/athena_resources.gyp:athena_resources',
      ],
      'sources': [
        'extensions/test/test_extensions_delegate.cc',
        'test/athena_test_base.cc',
        'test/athena_test_base.h',
        'test/athena_test_helper.cc',
        'test/athena_test_helper.h',
        'test/sample_activity.cc',
        'test/sample_activity.h',
        'test/sample_activity_factory.cc',
        'test/sample_activity_factory.h',
        'test/test_app_model_builder.cc',
        'test/test_app_model_builder.h',
        'test/test_resource_manager_delegate.cc',
        'wm/test/window_manager_impl_test_api.cc',
        'wm/test/window_manager_impl_test_api.h',
      ],
    },
    {
      'target_name': 'athena_unittests',
      'type': 'executable',
      'dependencies': [
        '../skia/skia.gyp:skia',
        '../testing/gtest.gyp:gtest',
        'athena_app_shell_lib',
        'athena_lib',
        'athena_test_support',
        'main/athena_main.gyp:athena_main_lib',
        'resources/athena_resources.gyp:athena_pak',
      ],
      'sources': [
        'activity/activity_manager_unittest.cc',
        'common/fill_layout_manager_unittest.cc',
        'content/app_activity_unittest.cc',
        'home/athena_start_page_view_unittest.cc',
        'home/home_card_gesture_manager_unittest.cc',
        'home/home_card_unittest.cc',
        'input/accelerator_manager_unittest.cc',
        'resource_manager/memory_pressure_notifier_unittest.cc',
        'resource_manager/resource_manager_unittest.cc',
        'screen/screen_manager_unittest.cc',
        'test/athena_unittests.cc',
        'wm/split_view_controller_unittest.cc',
        'wm/window_list_provider_impl_unittest.cc',
        'wm/window_manager_unittest.cc',
      ],
    }
  ],
}

