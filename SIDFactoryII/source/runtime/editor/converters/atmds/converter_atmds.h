/**
 *	ATMDS 3.2 "Converter" - IT IS NOT A CONVERTER
 *
 *	This "converter" is made to marry ATMDS 3.2 data files with the ATMDS 3.2 driver.
 */
#pragma once

#include "runtime/editor/components/component_console.h"
#include "runtime/editor/converters/converterbase.h"
#include "runtime/editor/converters/utils/sf2_interface.h"

#include <set>

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

		std::shared_ptr<Utility::C64File> tryUnpack(const void* inData, unsigned int inDataSize) const;
		bool maybeEditorLayout(std::shared_ptr<Utility::C64File>) const;

		// Console
		std::shared_ptr<ComponentConsole> m_Console;
	};
}
