#include "gpu/async_upload.h"

namespace Raster {
    std::mutex AsyncUpload::m_infoMutex;
    std::unordered_map<int, AsyncUploadInfo> AsyncUpload::m_infos;
    std::thread AsyncUpload::m_uploader;
    bool AsyncUpload::m_running = false;
    void* AsyncUpload::m_context;
    static int s_uploadIdCache = 0;

    AsyncUploadInfo::AsyncUploadInfo() {
        this->ready = false;
        this->executed = false;
    }

    void AsyncUpload::Initialize() {
        RASTER_LOG("booting up async uploader");
        m_running = true;
        m_context = GPU::ReserveContext();
        m_uploader = std::thread(AsyncUpload::UploaderLogic);
    }

    void AsyncUpload::Terminate() {
        m_running = false;
        m_uploader.join();
    }

    AsyncUploadInfoID AsyncUpload::GenerateTextureFromImage(std::shared_ptr<Image> image) {
        int uploadID = s_uploadIdCache++;
        AsyncUploadInfo info;
        info.image = image;
        info.ready = false;
        info.texture = Texture();

        SyncPutAsyncUploadInfo(uploadID, info);

        return uploadID;
    }

    void AsyncUpload::DestroyTexture(Texture texture) {
        AsyncUploadInfo info;
        info.deleteTexture = texture;
        SyncPutAsyncUploadInfo(s_uploadIdCache++, info);
    }

    bool AsyncUpload::IsUploadReady(AsyncUploadInfoID t_id) {
        if (SyncAsyncUploadInfoExists(t_id)) {
            auto info = SyncGetAsyncUploadInfo(t_id);
            return info.ready;
        }
        return false;
    }

    void AsyncUpload::DestroyUpload(AsyncUploadInfoID& t_id) {
        if (SyncAsyncUploadInfoExists(t_id)) {
            SyncDestroyAsyncUploadInfo(t_id);
            t_id = 0;
        }
    }

    AsyncUploadInfo& AsyncUpload::GetUpload(AsyncUploadInfoID t_id) {
        return SyncGetAsyncUploadInfo(t_id);
    }

    void AsyncUpload::UploaderLogic() {
        GPU::SetCurrentContext(m_context);

        std::vector<int> skipID;
        while (m_running) {
            if (SyncIsInfosEmpty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }
            // std::this_thread::sleep_for(std::chrono::milliseconds(10));
            auto pair = SyncGetFirstAsyncUploadInfo();
            auto& info = pair.second;
            if (info.executed) continue;

            info.executed = true;
            if (info.deleteTexture.handle) {
                GPU::DestroyTexture(info.deleteTexture);
                SyncDestroyAsyncUploadInfo(pair.first);
                continue;
            }

            TexturePrecision precision = TexturePrecision::Usual;
            if (info.image->precision == ImagePrecision::Half) precision = TexturePrecision::Half;
            if (info.image->precision == ImagePrecision::Full) precision = TexturePrecision::Full;

            auto generatedTexture = GPU::GenerateTexture(info.image->width, info.image->height, info.image->channels, precision, true);
            GPU::UpdateTexture(generatedTexture, 0, 0, info.image->width, info.image->height, info.image->channels, info.image->data.data());
            GPU::GenerateMipmaps(generatedTexture);
            GPU::Flush();

            info.texture = generatedTexture;
            info.image = nullptr;
            info.ready = true;

            SyncPutAsyncUploadInfo(pair.first, info);
        }
    }

    void AsyncUpload::SyncDestroyAsyncUploadInfo(AsyncUploadInfoID t_id) {
        std::lock_guard<std::mutex> lg(m_infoMutex);
        m_infos.erase(t_id);
    }

    void AsyncUpload::SyncPutAsyncUploadInfo(int t_key, AsyncUploadInfo t_info) {
        std::lock_guard<std::mutex> lg(m_infoMutex); 
        m_infos[t_key] = t_info;
    }

    AsyncUploadInfo& AsyncUpload::SyncGetAsyncUploadInfo(int t_key) {
        std::lock_guard<std::mutex> lg(m_infoMutex); 
        return m_infos[t_key];
    }

    bool AsyncUpload::SyncIsInfosEmpty() {
        std::lock_guard<std::mutex> lg(m_infoMutex); 
        if (m_infos.empty()) return true;
        for (auto& info : m_infos) {
            if (!info.second.executed) {
                return false; 
            }
        }
        return true;
    }

    std::pair<int, AsyncUploadInfo> AsyncUpload::SyncGetFirstAsyncUploadInfo() {
        std::lock_guard<std::mutex> lg(m_infoMutex); 
        return *m_infos.begin();
    }

    bool AsyncUpload::SyncAsyncUploadInfoExists(AsyncUploadInfoID t_id) {
        std::lock_guard<std::mutex> lg(m_infoMutex); 
        return m_infos.find(t_id) != m_infos.end();
    }
};