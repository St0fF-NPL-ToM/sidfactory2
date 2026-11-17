#include "runtime/editor/converters/atmds/converter_atmds.h"
#include "foundation/base/assert.h"
#include "runtime/editor/components_manager.h"
#include "runtime/editor/dialog/dialog_message.h"
#include "utils/c64file.h"

namespace Editor
{
	ConverterATMDS::ConverterATMDS()
	{
	}

	ConverterATMDS::~ConverterATMDS()
	{
	}

	const std::string ConverterATMDS::GetName() const { return "NULL converter"; }

	bool ConverterATMDS::CanConvert(const void* inData, unsigned int inDataSize) const
	{
		// Assert that there's is some data in the first place
		FOUNDATION_ASSERT(inData != nullptr);
		FOUNDATION_ASSERT(inDataSize > 0);

		/* Let's see - there should be 2 cases:
			- some .sid file with a header → which is the same as some .prg with an ATMDS-Player
			- some .prg file, which could be:
				- a CUNP-file from ATMDS saving
				- a ATMDS-Presentation file (should contain an unpacked music)
				- some other prg with an ATMDS-player - we should look out for its signatures
		*/
		const auto magic = *reinterpret_cast<const uint32_t*>(inData);
		std::shared_ptr<Utility::C64File> file;
		bool editor_layout = false;
		if (magic == 0x50534944 || magic == 0x52534944)
		{
			uint8_t* prgPtr = (uint8_t*)const_cast<void*>(inData) + 2;
			uint16_t prgBase = ((const uint16_t*)(inData))[4],
					 size = inDataSize - 2;
			if (prgBase == 0) // RSID includes the full .prg
				prgPtr += ((const uint16_t*)inData)[3],
					prgBase = ((const uint16_t*)inData)[((const uint16_t*)inData)[3] / 2];
			// now, prgPtr and prgBase point at the respective parts
			size -= (prgPtr - (const uint8_t*)inData); // and we know the bytes to follow.
			file = Utility::C64File::CreateFromData(prgBase, prgPtr, size);
		}
		else
			file = Utility::C64File::CreateFromPRGData(inData, inDataSize);

		// Now, we got both covered into the same container …
		if ((file->GetTopAddress() > 0x413c) && (file->GetBottomAddress() <= 0xb500))
		{ // should be a workstage file - if so (try unpack) - we can directly convert.
			uint8_t unpacked[0xb400 - 0x403c];
			uint16_t rpos { file->GetTopAddress() }, wpos;
			wpos = file->GetWord(rpos) - 0x403c, rpos += 2; // Zieladresse
			if (wpos)
				memset(unpacked, 0, 4); // reset speed (3.2 feature)
			const auto ctrl = file->GetByte(rpos++); // Controlbyte für RLE
			while (rpos < file->GetBottomAddress())
			{
				const auto b = file->GetByte(rpos++);
				if (b == ctrl)
					if (auto cnt = file->GetByte(rpos++))
						for (uint8_t c = file->GetByte(rpos++), i = 0; i < cnt; i++)
							unpacked[wpos++] = c;
					else
						break; // end mark found
				else
					unpacked[wpos++] = b;
			}
			file = Utility::C64File::CreateFromData(0x403c, unpacked, 0xb400 - 0x403c);
		} // unpacked into editor layout

		// check for possible editor layout or editor layout content
		// → starts before $4040/$403c
		// → end larger than first editor-sector
		if ((file->GetTopAddress() <= 0x4040) && (file->GetBottomAddress() > 0x5402) && (file->GetWord(0x4040) == 0x4900) && (file->GetWord(0x4042) == 0x4a00) && (file->GetWord(0x4044) == 0x4b00))
			// could contain an editor dump … in case the first song points to the correct tracks …
			return true;
		else
			return false;
	}


	bool ConverterATMDS::ConsumeKeyEvent(SDL_Keycode inKeyEvent, unsigned int inModifiers)
	{
		return false;
	}


	bool ConverterATMDS::Update()
	{
		FOUNDATION_ASSERT(GetState() != State::Uninitialized);

		if (GetState() == State::Initialized)
		{
			m_State = State::Completed;

			// Show a dialog, to test the flow!
			m_ComponentsManager->StartDialog(std::make_shared<DialogMessage>("ATMDS Converter", "You will now be converting this file to a null thing", 80, true, [&]() { }));
		}

		// Return true, to indicate that the conversion
		return true;
	}


	void ConverterATMDS::Setup()
	{
	}
}
