#include "Camera.h"

#include <algorithm>

using namespace DirectX;
using namespace SimpleMath;

Matrix Camera::GetViewMatrix() const
{
    return Matrix::CreateLookAt(m_Position, m_Position + m_Forward, Vector3::Up);
}

Matrix Camera::GetProjectionMatrix() const
{
    return Matrix::CreatePerspectiveFieldOfView(m_Fov, m_AspectRatio, m_NearPlane, m_FarPlane);
}

Matrix Camera::GetViewProjectionMatrix() const
{
    return GetViewMatrix() * GetProjectionMatrix();
}

BoundingFrustum Camera::GetFrustum() const
{
    BoundingFrustum frustum(GetProjectionMatrix());
    frustum.Transform(frustum, Matrix::CreateFromYawPitchRoll(m_Rotation) * 
        Matrix::CreateTranslation(m_Position));
    return frustum;
}

Vector3 Camera::GetPosition() const
{
    return m_Position;
}

void Camera::SetViewPort(ID3D11DeviceContext* context) const
{
    context->RSSetViewports(1, &m_Viewport);
}

void Camera::Update(const ImGuiIO& io)
{
    m_AspectRatio = io.DisplaySize.x / io.DisplaySize.y;
    m_Viewport = 
    {
        0.0f, 0.0f,
        io.DisplaySize.x, io.DisplaySize.y,
        0.0f, 1.0f
    };

    const float dt = io.DeltaTime;
    const Matrix rot = Matrix::CreateFromYawPitchRoll(m_Rotation);
    const auto forward = rot.Forward();
    forward.Normalize(m_Forward);
    const auto right = rot.Right();
    right.Normalize(m_Right);

    if (io.KeysDown[ImGui::GetKeyIndex(ImGuiKey_W)])
    {
        m_Position += forward * dt * 50.0f;
    }
    if (io.KeysDown[ImGui::GetKeyIndex(ImGuiKey_S)])
    {
        m_Position -= forward * dt * 50.0f;
    }
    if (io.KeysDown[ImGui::GetKeyIndex(ImGuiKey_A)])
    {
        m_Position -= right * dt * 50.0f;
    }
    if (io.KeysDown[ImGui::GetKeyIndex(ImGuiKey_D)])
    {
        m_Position += right * dt * 50.0f;
    }

    if (io.MouseDown[ImGuiMouseButton_Right])
    {
        const auto dx = io.MouseDelta.x;
        const auto dy = io.MouseDelta.y;
        m_Rotation.x -= dy * 0.003f;
        m_Rotation.y -= dx * 0.003f;
        m_Rotation.x = std::clamp(m_Rotation.x, -XM_PIDIV2 + 0.1f, XM_PIDIV2 - 0.1f);
    }
}
