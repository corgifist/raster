#include "common/attribute.h"
#include "common/randomizer.h"

namespace Raster {
    AttributeBase::AttributeBase() {
        
    }

    void AttributeBase::Initialize() {
        this->name = "New Attribute ";
        this->id = Randomizer::GetRandomInteger();
    }
};