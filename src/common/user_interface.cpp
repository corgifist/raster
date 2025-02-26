#include "common/user_interface.h"

namespace Raster {
    void UserInterfaceBase::Render() {
        AbstractRender();
    }

    Json UserInterfaceBase::Serialize() {
        Json result = Json::object();
        result["ID"] = id;
        result["PackageName"] = packageName;
        result["Data"] = AbstractSerialize();
        return result;
    }

    void UserInterfaceBase::Load(Json t_data) {
        this->id = t_data["ID"];
        this->packageName = t_data["PackageName"];
        AbstractLoad(t_data["Data"]);
    }
};