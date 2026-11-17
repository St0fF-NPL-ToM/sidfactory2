/**
 *	This is an attempt to add ATMDS3-Import to SIDFactoryII.
 *
 *	It does not make much sense to put lots of effort into this, as there were only 11 or so tunes
 *	created with AcidTracker 3.2 and its "++" variants (actually, the non-+++-variants were never
 *	released, so I suppose nobody will find a trace of it anywhere …)
 *
 *	Thus, I'll reduce this task to "getting workstages and editor layouts" converted.
 *	Why: workstage files can still be created by people.  They unpack into the socalled
 *	"EditorLayout" using a most simple RLE (see implementation below).  The future is safe ;)
 *	And 2nd point being: some releases used the "MusicPresentationCreator" I made some day.  This
 *	thing does not much else:
 *	→ unpack the editor data into editor layout and run the player, while displaying what it did.
 *	So, saving a memory dump after running such a music presentation leaves you with an editor
 *	layout of the respective music file.
 *
 *	If you ask "why presentation in editor layout?"
 *
 *	→ The exact reason SIDFactoryII does live-packing: no weird sounding surprises allowed ;)
 *
 */

#include "runtime/editor/converters/atmds/converter_atmds.h"
#include "foundation/base/assert.h"
#include "foundation/graphics/textfield.h"
#include "foundation/platform/iplatform.h"
#include "runtime/editor/components_manager.h"
#include "runtime/editor/converters/utils/sf2_interface.h"
#include "runtime/editor/dialog/dialog_message.h"
#include "utils/c64file.h"

#include "libraries/ghc/fs_std.h"

using namespace fs;

namespace Editor
{
	ConverterATMDS::ConverterATMDS()
	{
	}

	ConverterATMDS::~ConverterATMDS()
	{
	}

	const std::string ConverterATMDS::GetName() const { return "ATMDS converter"; }

	bool ConverterATMDS::CanConvert(const void* inData, unsigned int inDataSize) const
	{
		auto file = tryUnpack(inData, inDataSize);
		// check for possible editor layout or editor layout content
		// → starts before $4040/$403c
		// → end larger than first editor-sector
		if (maybeEditorLayout(file))
			return true;
		else
		{ /* TODO: better options like finding table addresses and sizes, maybe running emulation
			 until some breakpoint is hit for a re-check. */
			return false;
		}
	}


	bool ConverterATMDS::ConsumeKeyEvent(SDL_Keycode inKeyEvent, unsigned int inModifiers)
	{
		if (inKeyEvent >= SDLK_1 && inKeyEvent <= SDLK_4)
			return (m_song = inKeyEvent - SDLK_1), true;
		return false;
	}


	bool ConverterATMDS::Update()
	{
		if (GetState() == State::Initialized)
		{
			// SF2::Interface sf2(m_Platform, *m_Console);
			// m_State = State::Convert;
			// auto& out = (sf2.GetCout() << "\nATMDS-CONVERT:\n- load default driver:");

			// const path driver_path = m_Platform->Storage_GetDriversHomePath();
			// const path driver_path_and_filename = driver_path / "sf2driver11_05.prg";
			// const bool driver_loaded = sf2.LoadFile(driver_path_and_filename.string());
			// if (!driver_loaded)
			// 	return out << "\nFailed to load driver: " << driver_path_and_filename.string(),
			// 		   false;
			// else
			// { // I suppose the testing was already done, so tryUnpack succeeded before.
			// 	// Show a dialog, to test the flow!
			// 	m_ComponentsManager->StartDialog(std::make_shared<DialogMessage>("ATMDS Converter", "You will now be converting this file from ATMDS 3.2", 80, true, [&]()
			// 		{
			// 			out << "\n… unpacking to 'editor_layout'";
			// 			m_file = tryUnpack(m_Data, m_DataSize);
			// 			out << "\n… counting songs";
			// 			/* How can we tell if a song contains a song?
			// 				→	track 1 needs an end-marker at a position further down than #2
			// 					else, the song should be regarded as a scratch space
			// 			*/
			// 			int songs {};
			// 			for (int i = 0; i < 4; i++)
			// 			{
			// 				const auto ta = m_file->GetWord(0x4040 + 8 * i);
			// 				for (int j = 0; j < 256; j++)
			// 					if (m_file->GetByte(ta + j) >= 0xfe)
			// 					{
			// 						if (j >= 2)
			// 							++songs;
			// 						break;
			// 					}
			// 			}
			// 			out << "… found " << songs << " songs.";
			// 			if (songs > 1)
			// 			{
			// 				out << "Please type the number [1…4] of the song you'd like to import.";
			// 			} else m_song=0; }));
			// }
			return true;
		}
		else if (GetState() == State::Convert && m_song > -1 && m_file->IsValid())
		{
			return true;
		}
		else
			return false;
	}


	void ConverterATMDS::Setup()
	{
	}


	std::shared_ptr<Utility::C64File> ConverterATMDS::tryUnpack(const void* inData, unsigned int inDataSize) const
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
		if (magic == 0x50534944 || magic == 0x52534944)
		{
			uint8_t* prgPtr = (uint8_t*)const_cast<void*>(inData);
			const uint16_t prgOffs = ((const uint16_t*)inData)[3];
			uint16_t prgBase = ((const uint16_t*)(inData))[4],
					 size = inDataSize;
			if (prgBase == 0) // RSID includes the full .prg
				file = Utility::C64File::CreateFromPRGData(prgPtr + prgOffs, inDataSize - prgOffs);
			else // otherwise we received a load address
				file = Utility::C64File::CreateFromData(prgBase, prgPtr + prgOffs, inDataSize - prgOffs);
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
						break; // end mark found: this file is indeed convertible!
				else
					unpacked[wpos++] = b;
			}
			file = Utility::C64File::CreateFromData(0x403c, unpacked, 0xb400 - 0x403c);
		} // unpacked into editor layout

		// At this point we may have produced an ATMDS Editor Layout. If not, the returned
		// file simply contains input data, but the test on EditorLayout will fail.
		return file;
	}

	bool ConverterATMDS::maybeEditorLayout(std::shared_ptr<Utility::C64File> file) const
	{
		// could contain an editor dump … in case the first song points to tracks.
		// To make it more clear: all tracks are $100 spaced, so maybe checking the spacing is more robust in case I experimented in the past …
		bool inside = (file->GetTopAddress() <= 0x4040) && (file->GetBottomAddress() > 0x5402);
		bool trk1_ok = file->GetWord(0x4040) == 0x4800;
		bool trk2_ok = file->GetWord(0x4042) == 0x4900;
		bool trk3_ok = file->GetWord(0x4044) == 0x4a00;
		return inside && trk1_ok && trk2_ok && trk3_ok;
	}

}
