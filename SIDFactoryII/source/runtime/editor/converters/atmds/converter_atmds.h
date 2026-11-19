#pragma once

#include "runtime/editor/components/component_console.h"
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
		std::shared_ptr<Utility::C64File> tryUnpack(const void* inData, unsigned int inDataSize) const;
		bool maybeEditorLayout(std::shared_ptr<Utility::C64File>) const;

		std::shared_ptr<Utility::C64File> m_file;
		int m_song { -1 };
		// Console
		std::shared_ptr<ComponentConsole> m_Console;
	};
}
