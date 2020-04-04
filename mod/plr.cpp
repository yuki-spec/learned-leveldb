#include "plr.h"
#include "util.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>


double
get_slope(struct point p1, struct point p2) {
    return (p2.y - p1.y) / (p2.x - p1.x);
}

struct line
get_line(struct point p1, struct point p2) {
    double a = get_slope(p1, p2);
    double b = -a * p1.x + p1.y;
    struct line l{.a = a, .b = b};
    return l;
}

struct point
get_intersetction(struct line l1, struct line l2) {
    double a = l1.a;
    double b = l2.a;
    double c = l1.b;
    double d = l2.b;
    struct point p {(d - c) / (a - b), (a * d - b * c) / (a - b)};
    return p;
}

bool
is_above(struct point pt, struct line l) {
    return pt.y > l.a * pt.x + l.b;
}

bool
is_below(struct point pt, struct line l) {
    return pt.y < l.a * pt.x + l.b;
}

struct point
get_upper_bound(struct point pt, double gamma) {
    struct point p {pt.x, pt.y + gamma};
    return p;
}

struct point
get_lower_bound(struct point pt, double gamma) {
    struct point p {pt.x, pt.y - gamma};
    return p;
}

GreedyPLR::GreedyPLR(double gamma) {
    this->state = "need2";
    this->gamma = gamma;
}

int counter = 0;

struct segment
GreedyPLR::process(const struct point& pt) {
    this->last_pt = pt;
    if (this->state.compare("need2") == 0) {
        this->s0 = pt;
        this->state = "need1";
    } else if (this->state.compare("need1") == 0) {
        this->s1 = pt;
        setup();
        this->state = "ready";
    } else if (this->state.compare("ready") == 0) {
        return process__(pt);
    } else {
        // impossible
        std::cout << "ERROR in process" << std::endl;
    }
    struct segment s{
            .start = 0, .stop = 0, .slope = 0, .intercept = 0
    };
    return s;
}

void
GreedyPLR::setup() {
    this->rho_lower = get_line(get_upper_bound(this->s0, this->gamma),
                               get_lower_bound(this->s1, this->gamma));
    this->rho_upper = get_line(get_lower_bound(this->s0, this->gamma),
                               get_upper_bound(this->s1, this->gamma));
    this->sint = get_intersetction(this->rho_upper, this->rho_lower);
}

struct segment
GreedyPLR::current_segment() {
    double segment_start = this->s0.x;
    double segment_stop = this->last_pt.x;
    double avg_slope = (this->rho_lower.a + this->rho_upper.a) / 2.0;
    double intercept = -avg_slope * this->sint.x + this->sint.y;
    struct segment s{
            .start = segment_start,
            .stop = segment_stop,
            .slope = avg_slope,
            .intercept = intercept
    };
    return s;
}

struct segment
GreedyPLR::process__(struct point pt) {
    if (!(is_above(pt, this->rho_lower) && is_below(pt, this->rho_upper))) {
        struct segment prev_segment = current_segment();
        this->s0 = pt;
        this->state = "need1";
        return prev_segment;
    }

    struct point s_upper = get_upper_bound(pt, this->gamma);
    struct point s_lower = get_lower_bound(pt, this->gamma);
    if (is_below(s_upper, this->rho_upper)) {
        this->rho_upper = get_line(this->sint, s_upper);
    }
    if (is_above(s_lower, this->rho_lower)) {
        this->rho_lower = get_line(this->sint, s_lower);
    }
    struct segment s{
            .start = 0, .stop = 0, .slope = 0, .intercept = 0
    };
    return s;
}

struct segment
GreedyPLR::finish() {
    struct segment s{
            .start = 0, .stop = 0, .slope = 0, .intercept = 0
    };
    if (this->state.compare("need2") == 0) {
        this->state = "finished";
        return s;
    } else if (this->state.compare("need1") == 0) {
        this->state = "finished";
        s.start = this->s0.x;
        s.stop = this->s0.x + 1;
        s.slope = 0;
        s.intercept = this->s0.y;
        return s;
    } else if (this->state.compare("ready") == 0) {
        this->state = "finished";
        return current_segment();
    } else {
        std::cout << "ERROR in finish" << std::endl;
        return s;
    }
}

PLR::PLR(double gamma) {
    this->gamma = gamma;
}

std::deque <segment>&
PLR::train(std::deque<string>& keys, bool file) {
    GreedyPLR plr(this->gamma);
    int count = 0;
    size_t size = keys.size();
    for (int i = 0; i < size; ++i) {
        struct segment seg = plr.process(point((double) stoull(keys.front()), i));
        if (seg.start != 0 ||
            seg.stop != 0 ||
            seg.slope != 0 ||
            seg.intercept != 0) {
            this->segments.push_back(seg);
        }
        keys.pop_front();

        if (!file && ++count % 10 == 0 && adgMod::env->compaction_awaiting.load() != 0) {
            segments.clear();
            return segments;
        }
    }

    struct segment last = plr.finish();
    if (last.start != 0 ||
        last.stop != 0 ||
        last.slope != 0 ||
        last.intercept != 0) {
        this->segments.push_back(last);
    }

    return this->segments;
}
