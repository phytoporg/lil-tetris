typedef struct {
    Uint8 occupancy[4][4];
    Uint8 rows;
    Uint8 cols;
} Pattern;

static Pattern EmptyPattern = {
    { { 0, 0, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    0,
    0
};

static Pattern LShapePatternLeft = {
    { { 1, 1, 1, 0},
      { 1, 0, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3
};

static Pattern LShapePatternRight = {
    { { 1, 1, 1, 0},
      { 0, 0, 1, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3
};

static Pattern ZShapePatternLeft = {
    { { 1, 1, 0, 0},
      { 0, 1, 1, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3
};

static Pattern ZShapePatternRight = {
    { { 0, 1, 1, 0},
      { 1, 1, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3
};

static Pattern TShapePattern = {
    { { 0, 1, 0, 0},
      { 1, 1, 1, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    3
};

static Pattern LineShapePattern = {
    { { 1, 1, 1, 1},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    1,
    4
};

static Pattern SquareShapePattern = {
    { { 1, 1, 0, 0},
      { 1, 1, 0, 0},
      { 0, 0, 0, 0},
      { 0, 0, 0, 0} },
    2,
    2
};
