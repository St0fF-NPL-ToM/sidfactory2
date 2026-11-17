#pragma once

#include "runtime/editor/converters/converterbase.h"

namespace Editor
{
	class ConverterATMDS : public ConverterBase
	{
	public:
		ConverterATMDS();
		virtual ~ConverterATMDS();

		const std::string GetName() const override;
		bool CanConvert(const void* inData, unsigned int inDataSize) const override;
		bool ConsumeKeyEvent(SDL_Keycode inKeyEvent, unsigned int inModifiers) override;
		bool Update() override;

	private:
		void Setup() override;
	};
}
