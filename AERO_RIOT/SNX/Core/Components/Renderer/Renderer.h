#pragma once

#include <SNX/Core/Object/Component.h>
#include <SNX/Core/Object/RenderPass.h>

struct RenderContext;

class Renderer : public Component {
public:
	// inheriting/forwarding constructor
	using Component::Component;

	[[nodiscard]]
	bool IsVisible() const noexcept { return m_visible; }

	void SetVisible(bool visible) noexcept { m_visible = visible; }

	[[nodiscard]]
	virtual RenderPass GetRenderPass() const noexcept = 0;

protected:
	void OnRender(const RenderContext& context) override final {
		if (!m_visible) return;

		Draw(context);
	}

	virtual void Draw(const RenderContext& context) = 0;

private:
	bool m_visible = true;
};