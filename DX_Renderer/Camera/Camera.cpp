#include "Camera.hpp"

namespace DXR
{
    
    Camera::Camera(const XMFLOAT3& position, const XMFLOAT3& view_direction)
    {
        this->position = position;
        this->view_direction = view_direction;
    }
    
    XMMATRIX Camera::ViewMatrix() const
    {
        XMVECTOR focus_position = {position.x + view_direction.x,position.y + view_direction.y,position.z + view_direction.z,1.0f};
        return XMMatrixLookAtLH({position.x,position.y,position.z,1.0f},focus_position,{up_direction.x,up_direction.y,up_direction.z,1.0f});
    }
    
}
