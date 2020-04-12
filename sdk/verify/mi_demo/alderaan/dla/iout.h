#ifndef IOU_H
#define IOU_H

#include <vector>
#include <time.h>

 struct DetBBox {
    float x1, y1;
    float x2, y2;
    float score;
    float lm1_x, lm1_y;
    float lm2_x, lm2_y;
    float lm3_x, lm3_y;
    float lm4_x, lm4_y;
   float lm5_x, lm5_y;
} ;


struct TrackBBox
{
    // x-component of top left coordinate
    float x;
    // y-component of top left coordinate
    float y;
    // width of the box
    float w;
    // height of the box
    float h;
    // score of the box;
    float score;
    float lm1_x, lm1_y;
    float lm2_x, lm2_y;
    float lm3_x, lm3_y;
    float lm4_x, lm4_y;
    float lm5_x, lm5_y;
};

struct Track
{
    std::vector<TrackBBox> boxes;
    float max_score;
    int start_frame;
    int id;
};

class IOUTracker
{
public:
    IOUTracker();
    ~IOUTracker();
    float iou_sigma_l = 0;  // low detection threshold
    float iou_sigma_h = 0.2;  // high detection threshold
    float iou_sigma_iou = 0.4;  // IOU threshold
    float iou_sigma_show = 0.85;  // IOU threshold
    float iou_t_min = 5;  // minimum track length in frames
    float iou_t_max = 20;  // minimum track length in frames

    int max_id = 0;
    int track_id = 1; // Starting ID for the Tracks

    std::vector<Track> active_tracks;
    std::vector<Track> finished_tracks;

    // start IOU Tracker
    std::vector<Track> track_iou(std::vector< std::vector<TrackBBox> > detections);
};






#endif
