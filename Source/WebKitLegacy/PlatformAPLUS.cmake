list(APPEND WebKitLegacy_PRIVATE_INCLUDE_DIRECTORIES
    "${WEBCORE_DIR}/platform/mui"
    "${WEBCORE_DIR}/platform/bal"
    "${WEBCORE_DIR}/platform/graphics/morphos"
    "${WEBKITLEGACY_DIR}/morphos/ABPFilterParser"
    "${WEBKITLEGACY_DIR}/mui/WebCoreLeaks"
    "${WEBKITLEGACY_DIR}/mui/aplus"
    "${WEBKITLEGACY_DIR}/mui"
    "${WEBCORE_DIR}/platform/aplus"
    "${WEBCORE_DIR}/platform/aplus/compat"
    "${WebCore_DERIVED_SOURCES_DIR}"
    "${WEBKITLEGACY_DIR}/mui/WebCoreSupport"
    "${WEBKITLEGACY_DIR}/mui/Widgets"
)

list(APPEND WebKitLegacy_SOURCES

    mui/BackForwardList.cpp
    mui/DefaultPolicyDelegate.cpp
    mui/DOMCoreClasses.cpp
    mui/DOMCSSClasses.cpp
    mui/DOMEventsClasses.cpp
    mui/DOMHTMLClasses.cpp
    mui/DOMRange.cpp
    mui/HTTPHeaderPropertyBag.cpp
    mui/WebApplicationCache.cpp
    mui/WebBackForwardList.cpp
    mui/WebDatabaseManager.cpp
    mui/WebDataSource.cpp
    mui/WebDragData.cpp
    mui/WebDocumentLoader.cpp
    mui/WebDownload.cpp
    mui/WebError.cpp
    mui/WebFramePolicyListener.cpp
    mui/WebGeolocationPosition.cpp
    mui/WebIconDatabase.cpp
    mui/WebHistory.cpp
    mui/WebHistoryItem.cpp
    mui/WebHitTestResults.cpp
    mui/WebInspector.cpp
    mui/WebMutableURLRequest.cpp
    mui/WebNavigationAction.cpp
    mui/WebPasswordFormData.cpp
    mui/WebPasswordFormUtils.cpp
    mui/WebPreferences.cpp
    mui/WebResource.cpp
    mui/WebScriptObject.cpp
    mui/WebSecurityOrigin.cpp
    mui/WebFrame.cpp
    mui/WebUtils.cpp
    mui/WebView.cpp
    mui/aplus/WebViewPrivateAplus.cpp
    mui/WebURLResponse.cpp

    mui/WebCoreSupport/WebChromeClient.cpp
    mui/WebCoreSupport/WebContextMenuClient.cpp
    mui/WebCoreSupport/WebDesktopNotificationsDelegate.cpp
    mui/WebCoreSupport/WebDragClient.cpp
    mui/WebCoreSupport/WebEditorClient.cpp
    mui/WebCoreSupport/WebGeolocationClient.cpp
    mui/WebCoreSupport/WebFrameLoaderClient.cpp
    mui/WebCoreSupport/WebFrameNetworkingContext.cpp
    mui/WebCoreSupport/WebInspectorClient.cpp
    mui/WebCoreSupport/WebPlatformStrategies.cpp
    mui/WebCoreSupport/WebPluginInfoProvider.cpp
    mui/WebCoreSupport/WebProgressTrackerClient.cpp
    mui/WebCoreSupport/WebVisitedLinkStore.cpp

    mui/Widgets/WebDragDataMorphOS.cpp

    mui/Storage/WebDatabaseProvider.cpp

    mui/AdBlock/AdBlock.cpp
    mui/AdBlock/WayfarerAdBlock.cpp
    morphos/ABPFilterParser/ABPFilterParser.cpp
    morphos/ABPFilterParser/BloomFilter.cpp
    morphos/ABPFilterParser/cosmeticFilter.cpp
    morphos/ABPFilterParser/filter.cpp
    morphos/ABPFilterParser/hashFn.cpp

    Storage/StorageAreaImpl.cpp
    Storage/StorageAreaSync.cpp
    Storage/StorageNamespaceImpl.cpp
    Storage/StorageSyncManager.cpp
    Storage/StorageThread.cpp
    Storage/StorageTracker.cpp
    Storage/WebDatabaseProvider.cpp
    Storage/WebStorageNamespaceProvider.cpp

    WebCoreSupport/WebViewGroup.cpp
)

add_definitions("-include WebKitPrefix.h")

set(WebKitLegacy_LIBRARY_TYPE STATIC)
