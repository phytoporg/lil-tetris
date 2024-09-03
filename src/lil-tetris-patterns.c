typedef enum
{
    PATTERN_NONE,
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
    Uint8 colOffset;
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
    0
};
static Pattern* EmptyPatternRotations[1] = {
    &EmptyPattern
};

// L-Patterns
static Pattern LPatternLeft = {
    { { 1, 1, 1, 0},
      { 1, 0, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
    0
};

static Pattern LPatternLeft_Rot90 = {
    { { 1, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
    0
};

static Pattern LPatternLeft_Rot180 = {
    { { 0, 0, 1, 0},
      { 1, 1, 1, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
    0
};

static Pattern LPatternLeft_Rot270 = {
    { { 1, 0, 0, 0},
      { 1, 0, 0, 0},
      { 1, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
    0
};

static Pattern* LPatternLeftRotations[4] = {
    &LPatternLeft,
    &LPatternLeft_Rot90,
    &LPatternLeft_Rot180,
    &LPatternLeft_Rot270,
};

static Pattern LPatternRight = {
    { { 1, 1, 1, 0},
      { 0, 0, 1, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
    0
};

static Pattern LPatternRight_Rot90 = {
    { { 0, 1, 0, 0},
      { 0, 1, 0, 0},
      { 1, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
    0
};

static Pattern LPatternRight_Rot180 = {
    { { 1, 0, 0, 0},
      { 1, 1, 1, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
    0
};

static Pattern LPatternRight_Rot270 = {
    { { 1, 1, 0, 0},
      { 1, 0, 0, 0},
      { 1, 0, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
    0
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
    0
};

static Pattern ZPatternLeft_Rot90 = {
    { { 0, 1, 0, 0},
      { 1, 1, 0, 0},
      { 1, 0, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
    0
};
static Pattern* ZPatternLeftRotations[2] = {
    &ZPatternLeft,
    &ZPatternLeft_Rot90
};

static Pattern ZPatternRight = {
    { { 0, 1, 1, 0},
      { 1, 1, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
    0
};

static Pattern ZPatternRight_Rot90 = {
    { { 1, 0, 0, 0},
      { 1, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    2,
    0
};
static Pattern* ZPatternRightRotations[2] = {
    &ZPatternRight,
    &ZPatternRight_Rot90,
};

static Pattern TPattern = {
    { { 0, 1, 0, 0},
      { 1, 1, 1, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3,
    0
};

static Pattern TPattern_Rot90 = {
    { { 0, 1, 0, 0},
      { 0, 1, 1, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    3,
    1
};

static Pattern TPattern_Rot180 = {
    { { 0, 0, 0, 0},
      { 1, 1, 1, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    3,
    0
};

static Pattern TPattern_Rot270 = {
    { { 0, 1, 0, 0},
      { 1, 1, 0, 0},
      { 0, 1, 0, 0},
      { 0, 0, 0, 0} },
    3,
    3,
    0
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
    0
};

static Pattern LinePattern_Rot90 = {
    { { 1, 0, 0, 0},
      { 1, 0, 0, 0},
      { 1, 0, 0, 0},
      { 1, 0, 0, 0} },
    4,
    1,
    0
};
static Pattern* LinePatternRotations[2] = {
    &LinePattern,
    &LinePattern_Rot90,
};

static Pattern SquarePattern = {
    { { 1, 1, 0, 0},
      { 1, 1, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    2,
    0
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
