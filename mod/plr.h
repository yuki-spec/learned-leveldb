#include <string>
#include <vector>


struct point {
    double x;
    double y;

    point() = default;
    point(double x, double y) : x(x), y(y) {}
};

struct line {
    double a;
    double b;
};

struct segment {
    double start;
    double stop;
    double slope;
    double intercept;
};

double get_slope(struct point p1, struct point p2);
struct line get_line(struct point p1, struct point p2);
struct point get_intersetction(struct line l1, struct line l2);

bool is_above(struct point pt, struct line l);
bool is_below(struct point pt, struct line l);

struct point get_upper_bound(struct point pt, double gamma);
struct point get_lower_bound(struct point pt, double gamma);


class GreedyPLR {
private:
    std::string state;
    double gamma;
    struct point last_pt;
    struct point s0;
    struct point s1;
    struct line rho_lower;
    struct line rho_upper;
    struct point sint;

    void setup();
    struct segment current_segment();
    struct segment process__(struct point pt);

public:
    GreedyPLR(double gamma);
    struct segment process(struct point& pt);
    struct segment finish();
};

class PLR {
private:
    double gamma;
    std::vector<struct segment> segments;

public:
    PLR(double gamma);
    std::vector<struct segment>& train(std::vector<struct point>& points, bool file);
//    std::vector<double> predict(std::vector<double> xx);
//    double mae(std::vector<double> y_true, std::vector<double> y_pred);
};
