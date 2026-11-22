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
#include "runtime/editor/dialog/dialog_message.h"
#include "runtime/emulation/cpumemory.h"
#include "utils/c64file.h"

#include "libraries/ghc/fs_std.h"
#include "utils/utilities.h"

using namespace fs;

namespace Editor
{
	ConverterATMDS::ConverterATMDS()
	{
	}

	ConverterATMDS::~ConverterATMDS()
	{
	}

	const std::string ConverterATMDS::GetName() const { return "ATMDS-3.2'26 converter"; }

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
		return false;
	}


	bool ConverterATMDS::Update()
	{
		bool result { false };

		if (GetState() == State::Initialized)
		{
			ConsoleOStreamBuffer outstream(&(*m_Console));
			std::ostream cout(&outstream);
			m_State = State::Convert;

			// Begin the "conversion" by preparing the driver
			const path driver_path = m_Platform->Storage_GetDriversHomePath();
			const path driver_path_and_filename = driver_path / "AcidTracker 3.2-26.prg";
			cout << "\nATMDS-3.2-CONVERT:\n\n- load default driver: " << driver_path_and_filename.string();

			void* data = nullptr;
			long data_size = 0;
			if (Utility::ReadFile(driver_path_and_filename.string(), 65536, &data, data_size) && data_size > 2)
			{
				// Expected situation:
				//	- we have 2 files: the driver in "data" and the music file in "m_data"
				//  - tryUnpack already succeeded, as well as MaybeEditorLayout
				//
				//	Thus, the simpe solution is to create memory space, insert all data accordingly
				//	and finally using the utilities to produce the required result.
				uint8_t c64[0x10000] {};

				// produce readable constants
				const uint8_t* drv_prg = reinterpret_cast<uint8_t*>(data) + 2;
				const uint16_t drv_start = Utility::C64File::ReadTargetAddressFromData(data, data_size);
				const uint16_t drv_size = data_size - 2;
				const uint16_t drv_end = drv_start + drv_size;

				// insert driver
				memcpy(c64 + drv_start, drv_prg, drv_size);
				cout << "\n- inserted driver into memory-region: " << std::hex << drv_start << "-" << drv_end << std::dec;

				// tryUnpack already succeeded, so further testing can be kept minimal
				cout << "\n- unpacking input file to 'editor_layout'";
				auto file = tryUnpack(m_Data, m_DataSize);
				// insert data
				memcpy(c64 + file->GetTopAddress(), file->GetData(), file->GetDataSize());
				cout << " ... and inserted memory-region: " << file->GetTopAddress() << "-" << file->GetBottomAddress();

				// determine "real" end of file
				const uint16_t end = std::max(drv_end, file->GetBottomAddress());

				// set up result
				m_Result = Utility::C64File::CreateFromData(drv_start, c64 + drv_start, end - drv_start);

				cout << "\n- combined driver+data: " << m_Result->GetTopAddress() << "-" << m_Result->GetBottomAddress();
				m_State = State::Completed;
				result = true;
			}
			else
				cout << "\n! FAILURE TO READ THE DRIVER FILE!\n";
			if (data)
				delete[] static_cast<char*>(data); // clean up asap
		}
		return result;
	}


	void ConverterATMDS::Setup()
	{
		const auto& dimensions = m_TextField->GetDimensions();
		m_Console = std::make_shared<ComponentConsole>(0, 0, nullptr, m_TextField, 1, 2, dimensions.m_Width - 2, dimensions.m_Height - 5);
		m_ComponentsManager->AddComponent(m_Console);
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
