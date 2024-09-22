#include <SDL2/SDL.h>

typedef enum
{
    PATTERN_NONE = 0,
    PATTERN_L_L,
    PATTERN_L_R,
    PATTERN_Z_L,
    PATTERN_Z_R,
    PATTERN_T_SHAPE,
    PATTERN_LINE_SHAPE,
    PATTERN_SQUARE_SHAPE,
    PATTERN_MAX_VALUE
} PatternType_t;

typedef struct {
    Uint8 occupancy[4][4];
    Uint8 rows;
    Uint8 cols;
} Pattern;

#define PATTERN_ROTATION_COUNT(PatternRotations) \
    (sizeof(PatternRotations) / sizeof(PatternRotations[0]))

// Empty patterns
static Pattern EmptyPattern = {
    { { 0, 0, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    0,
    0,
};
static Pattern* EmptyPatternRotations[1] = {
    &EmptyPattern
};

// Pattern orientations and wall kick data sourced from the Super Rotation System
// documented at harddrop.com/wiki/SRS

// L-Patterns
static Pattern LPatternLeft = {
    { { 0, 0, 1, 0},
      { 1, 1, 1, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
};

static Pattern LPatternLeft_Rot90 = {
    { { 0, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 1, 1, 0},
      { 0, 0, 0, 0} },
    3,
    2,
};

static Pattern LPatternLeft_Rot180 = {
    { { 0, 0, 0, 0},
      { 1, 1, 1, 0},
      { 1, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
};

static Pattern LPatternLeft_Rot270 = {
    { { 1, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
};

static Pattern* LPatternLeftRotations[4] = {
    &LPatternLeft,
    &LPatternLeft_Rot90,
    &LPatternLeft_Rot180,
    &LPatternLeft_Rot270,
};

static Pattern LPatternRight = {
    { { 1, 0, 0, 0},
      { 1, 1, 1, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
};

static Pattern LPatternRight_Rot90 = {
    { { 0, 1, 1, 0},
      { 0, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
};

static Pattern LPatternRight_Rot180 = {
    { { 0, 0, 0, 0},
      { 1, 1, 1, 0},
      { 0, 0, 1, 0},
      { 0, 0, 0, 0} },
    3,
    3,
};

static Pattern LPatternRight_Rot270 = {
    { { 0, 1, 0, 0},
      { 0, 1, 0, 0},
      { 1, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
};

static Pattern* LPatternRightRotations[4] = {
    &LPatternRight,
    &LPatternRight_Rot90,
    &LPatternRight_Rot180,
    &LPatternRight_Rot270,
};

static Pattern ZPatternLeft = {
    { { 1, 1, 0, 0},
      { 0, 1, 1, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
};

static Pattern ZPatternLeft_Rot90 = {
    { { 0, 0, 1, 0},
      { 0, 1, 1, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
};

static Pattern ZPatternLeft_Rot180 = {
    { { 0, 0, 0, 0},
      { 1, 1, 0, 0},
      { 0, 1, 1, 0},
      { 0, 0, 0, 0} },
    2,
    3,
};

static Pattern ZPatternLeft_Rot270 = {
    { { 0, 1, 0, 0},
      { 1, 1, 0, 0},
      { 1, 0, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
};

static Pattern* ZPatternLeftRotations[4] = {
    &ZPatternLeft,
    &ZPatternLeft_Rot90,
    &ZPatternLeft_Rot180,
    &ZPatternLeft_Rot270
};

static Pattern ZPatternRight = {
    { { 0, 1, 1, 0},
      { 1, 1, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
};

static Pattern ZPatternRight_Rot90 = {
    { { 0, 1, 0, 0},
      { 0, 1, 1, 0},
      { 0, 0, 1, 0},
      { 0, 0, 0, 0} },
    3,
    2,
};

static Pattern ZPatternRight_Rot180 = {
    { { 0, 0, 0, 0},
      { 0, 1, 1, 0},
      { 1, 1, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
};

static Pattern ZPatternRight_Rot270 = {
    { { 1, 0, 0, 0},
      { 1, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
};

static Pattern* ZPatternRightRotations[4] = {
    &ZPatternRight,
    &ZPatternRight_Rot90,
    &ZPatternRight_Rot180,
    &ZPatternRight_Rot270,
};

static Pattern TPattern = {
    { { 0, 1, 0, 0},
      { 1, 1, 1, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
};

static Pattern TPattern_Rot90 = {
    { { 0, 1, 0, 0},
      { 0, 1, 1, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    3,
};

static Pattern TPattern_Rot180 = {
    { { 0, 0, 0, 0},
      { 1, 1, 1, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    3,
};

static Pattern TPattern_Rot270 = {
    { { 0, 1, 0, 0},
      { 1, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    3,
};

static Pattern* TPatternRotations[4] = {
    &TPattern,
    &TPattern_Rot90,
    &TPattern_Rot180,
    &TPattern_Rot270,
};

static Pattern LinePattern = {
    { { 1, 1, 1, 1},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    1,
    4,
};

static Pattern LinePattern_Rot90 = {
    { { 0, 0, 1, 0},
      { 0, 0, 1, 0},
      { 0, 0, 1, 0},
      { 0, 0, 1, 0} },
    4,
    1,
};

static Pattern LinePattern_Rot180 = {
    { { 0, 0, 0, 0},
      { 0, 0, 0, 0},
      { 1, 1, 1, 1},
      { 0, 0, 0, 0} },
    1,
    4,
};

static Pattern LinePattern_Rot270 = {
    { { 0, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 1, 0, 0} },
    1,
    4,
};

static Pattern* LinePatternRotations[4] = {
    &LinePattern,
    &LinePattern_Rot90,
    &LinePattern_Rot180,
    &LinePattern_Rot270,
};

static Pattern SquarePattern = {
    { { 1, 1, 0, 0},
      { 1, 1, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    2,
};
static Pattern* SquarePatternRotations[1] = {
    &SquarePattern,
};

static int PatternNumRotations[(int)PATTERN_MAX_VALUE] = {
    PATTERN_ROTATION_COUNT(EmptyPatternRotations),  // PATTERN_NONE,
    PATTERN_ROTATION_COUNT(LPatternLeftRotations),  // PATTERN_L_L,
    PATTERN_ROTATION_COUNT(LPatternRightRotations), // PATTERN_L_R,
    PATTERN_ROTATION_COUNT(ZPatternLeftRotations),  // PATTERN_Z_L,
    PATTERN_ROTATION_COUNT(ZPatternRightRotations), // PATTERN_Z_R,
    PATTERN_ROTATION_COUNT(TPatternRotations),      // PATTERN_T_SHAPE,
    PATTERN_ROTATION_COUNT(LinePatternRotations),   // PATTERN_LINE_SHAPE,
    PATTERN_ROTATION_COUNT(SquarePatternRotations), // PATTERN_SQUARE_SHAPE,
};

// Wall kick test matrices
typedef struct {
    Sint8 X;
    Sint8 Y;
} WallKickVector2;

typedef enum {
    WALLKICK_DIRECTION_RIGHT,
    WALLKICK_DIRECTION_LEFT,
} WallKickRotateDirection;

#define PATTERN_MAX_KICK_TESTS 5

// Index into here by target rotation index

// J, L, S, T, Z tetromino tests
// Right rotation
const WallKickVector2 PatternNonLineWallKickRightRotationTests[4][PATTERN_MAX_KICK_TESTS] = {
    { { 0, 0 }, {-1, 0}, {-1,  1 }, { 0,-2 }, {-1,-2 } }, // 270 ->   0
    { { 0, 0 }, {-1, 0}, {-1, -1 }, { 0, 2 }, {-1, 2 } }, //   0 ->  90
    { { 0, 0 }, { 1, 0}, { 1,  1 }, { 0,-2 }, { 1,-2 } }, //  90 ->  180
    { { 0, 0 }, { 1, 0}, { 1, -1 }, { 0, 2 }, { 1, 2 } }, // 180 ->  270
};

// Left rotation
const WallKickVector2 PatternNonLineWallKickLeftRotationTests[4][PATTERN_MAX_KICK_TESTS] = {
    { { 0, 0 }, { 1, 0}, { 1,  1 }, { 0,-2 }, { 1,-2 } }, //  90 ->   0
    { { 0, 0 }, {-1, 0}, {-1, -1 }, { 0, 2 }, {-1, 2 } }, // 180 ->  90
    { { 0, 0 }, {-1, 0}, {-1,  1 }, { 0,-2 }, {-1,-2 } }, // 270 -> 180
    { { 0, 0 }, { 1, 0}, { 1, -1 }, { 0, 2 }, { 1, 2 } }, //   0 -> 270
};

// I-piece tetromino tests
// Right rotation
const WallKickVector2 PatternLineWallKickRightRotationTests[4][PATTERN_MAX_KICK_TESTS] = {
    { { 0, 0 }, { 1, 0}, {-2, 0 }, { 1, 2 }, {-2,-1 } }, // 270 ->   0
    { { 0, 0 }, {-2, 0}, { 1, 0 }, {-2, 1 }, { 1,-2 } }, //   0 ->  90
    { { 0, 0 }, {-1, 0}, { 2, 0 }, {-1,-2 }, { 2, 1 } }, //  90 ->  180
    { { 0, 0 }, { 2, 0}, {-1, 0 }, { 2,-1 }, {-1, 2 } }, // 180 ->  270
};

// Left rotation
const WallKickVector2 PatternLineWallKickLeftRotationTests[4][PATTERN_MAX_KICK_TESTS] = {
    { { 0, 0 }, { 2, 0}, {-1, 0 }, { 2,-1 }, {-1, 2 } }, //  90 ->   0
    { { 0, 0 }, { 1, 0}, {-2, 0 }, { 1, 2 }, {-2,-2 } }, // 180 ->  90
    { { 0, 0 }, {-2, 0}, { 1, 0 }, {-2, 1 }, { 1,-2 } }, // 270 -> 180
    { { 0, 0 }, {-1, 0}, { 2, 0 }, {-1,-2 }, { 2, 1 } }, //   0 -> 270
};
