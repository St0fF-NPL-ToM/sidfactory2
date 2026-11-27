
# SIDFactoryII "open driver interface"

*By: Stefan < St0fF / NPL ^ ToM > Kaps*

<!-- TOC -->

- [Existing abstraction layer structure](#existing-abstraction-layer-structure)
    - [Music information abstraction layer](#music-information-abstraction-layer)
    - [Instrument abstraction layer](#instrument-abstraction-layer)
- [Necessary abstraction enhancements](#necessary-abstraction-enhancements)
    - [Music information level](#music-information-level)
        - [Assembling the music level](#assembling-the-music-level)
    - [Instrument information level](#instrument-information-level)

<!-- /TOC -->

SIDFactoryII was the first Cross-SID-Tracker I saw that was able to load different drivers according to the required situation.

Meaning:

- you can load a minimal driver for size coding productions
- you can use a trimmed-down driver for musically scoring rastertime-hungry effects
- and you can load up "the fully fledged driver"

At that moment I did not realize how much all those SF2-drivers had in common, but I have been deep-diving this topic since 1995, so already the option of exchanging the driver was maximally appealing to me.

Recent works show, that in theory with the right abstraction level, a cross-tracker could even be written, that supports ANY C64-drivers.  Although it requires a lot of code or at least pseudocode on the interface side to describe the abstraction of a complex driver.

Anyhow, with this know-how in the back I sought out to find what I could do about SIDFactoryII.

As a "small first goal" supporting ATMDS 3.2 Tunes loading and playback was selected.

---

## Existing abstraction layer structure

SIDFactoryII achieves its freedom of drivers by a certain abstraction structure.

### Music information abstraction layer

It requires a driver to use a 2-layer event system consisting of `OrderLists` and `Sequences`. That is actually a requirement coming directly from combining the target hardware and the target goal:

- 8 Bit CPU
- 1MHz - so really quick audio signal production is needed
- restricted Memory
- fixed callback rate → a fixed time quantum exists
- produce music, e.g. repetitive patterns of melody and rhythm

All these requirements lead to the insight, that a 2 layered system does make the most sense:

- repetitive patterns are the 2nd level: sequences.  describing music events.
- controlling the playback order of such sequences - that's the OrderLists, the layer above.

This sounds too easy to be "the whole picture" ;)

`OrderLists` and `Sequences` are each optimized and not really "open for expansion".

- OrderLists really only show order and transposition of sequences
- Sequences carry the complete set of state changing commands

Other trackers / drivers on the C64 for example allow commands in OrderLists, but provide less commands inside Sequences, or even a completely different command model to begin with.

### Instrument abstraction layer

The instrument abstraction layer is already "partially open".

SF2 requires an instrument list and a description for it.  This instrument list allows to connect to other tables via it's columns and special markers.  Also there is support for bitfields in the instrument description.

The other tables are simply described by their memory layout.  They all require only one "LOOP" sentinel in their first column to denote a table loop / end of effect.

So - in case "the driver to support" only uses this simple approach on tables, it is already sufficient.

---

## Necessary abstraction enhancements

As noted, to be "more open" we need to extend the existing abstractions.  We'll derive the necessary steps by looking closer at the internal interactions inside SF2.

### Music information level

`OrderLists` in SF2 are the absolute minimum necessary to produce music an not blow the memory requirements.

Also the fact, that each "channel" gets its own order list results from the memory restrictions, as this results in better SNR, less memory required for the same content.  This statement may be arguable and I'd be pleased to develop proof for it - if somebody financed this research …

> [!NOTE]
> The proof I created around 1997-2000, sadly, is lost.  I did a lot of experiments on optimal memory structuring, with lots of failures producing music that was up to 2,5x as large, compared to using this approach.  This in fact is the reason, Acid Tracker went from major 1.x to major 3.x without a major 2.x in between ;).

Anyhow, `sequences` provide:

- a parameter (instrument number, tie note, empty),
- a command (may be empty), and
- a payload ( note to play, pause, gate-switch, reset -- or in SF2: empty, note, gate ).

This information structure actually should provide the basis. Order Rows can carry exactly the same items of information (and, in fact, in other drivers they do), as Sequence Rows.

As SF2 internally works with `datasources`, the simple approach is to glue a base class in between the datasource classes of order list / sequence and their current parent class.

**So this is the first enhancement:** OrderLists and Sequences will get unified into `DataSourceEvents`.

***But how to describe Events?***

This is the next question to answer.  SF2 uses special byte values to denote explicit states.  F.e.: a note value of 0 means "no note, play a pause" inside a Sequence's payload.

Also - already implemented as an implicit rule by SF2 - "end of something"-markers are inside the parameter area, and in case they carry additional information (a loop pointer f.e.), this information is encoded as payload.

So - obviously we have `values` that are restricted by their respective `ranges`.  And we do have those markers or special values I'd like to call `sentinels`.

`Sentinels` carry semantics - at least to me this seems to be their primary design goal.  Semantics can be fixed at compile time, thus we simply have to define them:

- `SentinelType::NoSentinel` - a placeholder to be returned on queries in case a value is set
- `SentinelType::Empty` - neither a value, nor a sentinel is set
- `SentinelType::Stop` - this is the non-looping end mark sentinel
- `SentinelType::Loop` - well ...
- `SentinelType::Start` - this is special: SF2 does not yet support "switching to another song"
- `SentinelType::Legato` - this is the "tie note" sentinel
- `SentinelType::Gate` - this is the "+++" sentinel of SF2.  Other trackers use "flipping pauses", thus may have more than one Sentinel of this type
- `SentinelType::Mark` - exactly ... a not further specifyable special sentinel (like "xxx" in ATMDS)
- `SentinelType::Mark1`, `SentinelType::Mark2`, `SentinelType::Mark3` - in case a driver even needs more …

There need to be rules.  Some of them are already implicitly implemented:

- STOP | LOOP | START | LEGATO are reserved for the parameter item, as they all make the row a transparent event which consumes "no time"
- GATE has the SF2 special note-row semantic, thus only allowed as payload
- EMPTY and all MARKS are free to be used everywhere - only SF2 may not know how to edit those ...

---

#### Assembling the music level

`IDataSourceEvents` is created and inserted as an intermediate base between `DataSourceCPUMemory` and `DataSourceOrderList`|`DataSourceSequence`.

Furthermore, we've discussed Sentinels and the content of any "row".  But how do they actually play together?

The `IDriverArchitecture` receives 2 event definitions, describing how the driver uses the events.

They contain:

- for all 3 "row-elements": `parameter`, `command`, `payload`
  - a value range for each
  - sentinels for each
  - values to encode the sentinels
  - a textual representation how a sentinel should be rendered
- pure virtual functions to pack and unpack a data set and find an index position inside packed data

Every driver has it's own encoding of events.  This driver-specific know-how was implemented inside the data sources - now its moved into the event definitions.

- derive an `IEventDefinition`:
  - implement a CTor setting up sentinels and values
  - implement the respective `Pack`, `Unpack`, and `GetNextIndexPackedPosition` pure-virtual functions
- repeat for the other level of music data

That way, the gears begin to grip: the data sources are created referencing the respective event descriptions.  Therefore, receiving the functions and the information at once, and it stays constant for the lifecycle of the driver.

---

### Instrument information level

The instrument and table definitions of SF2 are already quite open.  There is the required table description and its accompanying instrument descriptions:

For each table:

- row/column counts and memory layout
- Name, Type and ID
- Rules for the respective table editor (coloring, ins/del, editing rules)
- even allows for "naming table rows" by providing a "text size" (actually meant to name instruments and effects)

Combining the instrument table with other tables:

- reference column of instrument
- special values
- bitmask-checks
- table loop condition description

This was more a small overview, what's already there.  Let's do a deeper check-up "how open" this actually is.

Those basic table definitions are very clear and optimal.  The memory layout and positioning is clear, naming and editor placement also work great.

But the tables are still "dumb":  The connection interface between instrument table uses the following information.

- TableID,
- InstrumentDataPointerPosition,
- PointerAndValue,
- InstrumentDataConditionalValuePosition,
- ConditionValueAndValue,
- ConditionEqualityValue,
- TableDataType,
- TableJumpMarkerValuePosition,
- TableJumpMarkerValue,
- TableJumpDestinationIndexPosition

This covers one conditional bitmask, and it covers to Mask the instrument table entry before using it as a pointer into the table.

Sufficient for the way JCH's players handle instruments, but far from being abstract.

For example: *Pulse Table activation in ATMDS-3.2*

The instrument's pulse pointer may have any uint8_t value. 2 Values are special:

- 0x00: disabled    - no pulse required at all
- 0xff: continuous  - continue previously running effect, e.g. skip init/turn-off

ATMDS3 does not have an "instrument engine enabler bit mask".  Instead it uses sentinel values for this decision.
