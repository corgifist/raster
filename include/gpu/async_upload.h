#pragma once

#include "gpu.h"
#include "common/randomizer.h"
#include "raster.h"
#include "image/image.h"

namespace Raster {

    struct AsyncUploadInfo {
        Texture texture;
        Texture deleteTexture;
        std::shared_ptr<Image> image;
        bool ready;
        bool executed;

        AsyncUploadInfo();
    };

    using AsyncUploadInfoID = int;

    struct AsyncUpload {
    public:
        static void Initialize();
        static void Terminate();
        static void UploaderLogic();

        static AsyncUploadInfoID GenerateTextureFromImage(std::shared_ptr<Image> t_image);
        static void DestroyTexture(Texture texture);

        static bool IsUploadReady(AsyncUploadInfoID t_id);
        static void DestroyUpload(AsyncUploadInfoID& t_id);
        static AsyncUploadInfo& GetUpload(AsyncUploadInfoID t_id);

    private:
        static void SyncDestroyAsyncUploadInfo(AsyncUploadInfoID t_id);
        static void SyncPutAsyncUploadInfo(int t_key, AsyncUploadInfo t_info);
        static AsyncUploadInfo& SyncGetAsyncUploadInfo(int t_key);
        static bool SyncIsInfosEmpty();
        static std::pair<int, AsyncUploadInfo> SyncGetFirstAsyncUploadInfo();
        static bool SyncAsyncUploadInfoExists(AsyncUploadInfoID t_id);

        static bool m_running;
        static std::mutex m_infoMutex;
        static std::unordered_map<int, AsyncUploadInfo> m_infos;
        static std::thread m_uploader;
    };
};