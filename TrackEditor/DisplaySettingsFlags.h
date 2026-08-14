#ifndef _TRACKEDITOR_DISPLAYSETTINGSFLAGS_H
#define _TRACKEDITOR_DISPLAYSETTINGSFLAGS_H
//-------------------------------------------------------------------------------------------------
// The editor's own display bitmask, split out of DisplaySettings.h so code
// that translates it does not have to pull in the generated Qt UI header.
// These values are persisted in QSettings under "show_models"; do not
// renumber them.
//-------------------------------------------------------------------------------------------------
#define SHOW_CENTER_SURF_MODEL     0x00000001
#define SHOW_CENTER_WIRE_MODEL     0x00000002
#define SHOW_LSHOULDER_SURF_MODEL  0x00000004
#define SHOW_LSHOULDER_WIRE_MODEL  0x00000008
#define SHOW_RSHOULDER_SURF_MODEL  0x00000010
#define SHOW_RSHOULDER_WIRE_MODEL  0x00000020
#define SHOW_LWALL_SURF_MODEL      0x00000040
#define SHOW_LWALL_WIRE_MODEL      0x00000080
#define SHOW_RWALL_SURF_MODEL      0x00000100
#define SHOW_RWALL_WIRE_MODEL      0x00000200
#define SHOW_ROOF_SURF_MODEL       0x00000400
#define SHOW_ROOF_WIRE_MODEL       0x00000800
#define SHOW_OWALLFLOOR_SURF_MODEL 0x00001000
#define SHOW_OWALLFLOOR_WIRE_MODEL 0x00002000
#define SHOW_LLOWALL_SURF_MODEL    0x00004000
#define SHOW_LLOWALL_WIRE_MODEL    0x00008000
#define SHOW_RLOWALL_SURF_MODEL    0x00010000
#define SHOW_RLOWALL_WIRE_MODEL    0x00020000
#define SHOW_LUOWALL_SURF_MODEL    0x00040000
#define SHOW_LUOWALL_WIRE_MODEL    0x00080000
#define SHOW_RUOWALL_SURF_MODEL    0x00100000
#define SHOW_RUOWALL_WIRE_MODEL    0x00200000
#define SHOW_SELECTION_HIGHLIGHT   0x00400000
#define SHOW_AILINE_MODELS         0x00800000
/* Retired: the environment floor was the green plane under the track, drawn
 * before the preview showed the real horizon. The bit stays defined and
 * unused, because saved profiles still contain it and reusing it would tie a
 * new checkbox to whatever the user last left this one at. */
#define SHOW_ENVIRONMENT_RETIRED   0x01000000
#define SHOW_TEST_CAR              0x02000000
#define SHOW_SIGNS                 0x04000000
#define SHOW_AUDIO                 0x08000000
#define SHOW_STUNTS                0x10000000
#define SHOW_CENTER_LINE           0x20000000
#define SHOW_REF_MODEL             0x40000000
#define SHOW_REF_WIRE_MODEL        0x80000000

// The original show_models word is full. New display features are persisted
// in a separate QSettings word named "show_features". These values are part
// of the saved editor profile too, so do not renumber or reuse them.
#define SHOW_FEATURE_TOWERS        0x00000001
//-------------------------------------------------------------------------------------------------
#endif
