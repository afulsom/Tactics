#pragma once
#include <string>
#include "Engine\Core\Rgba.hpp"


struct Message
{
	Message(std::string text, const Rgba& color, float size)
		: m_text(text), m_color(color), m_size(size) {}

	std::string m_text;
	Rgba m_color;
	float m_size;
};
