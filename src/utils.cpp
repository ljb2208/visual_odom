#include "utils.h"
#include "evaluate_odometry.h"

double getAbsoluteScale(int frame_id)    {
  std::string line;
  int i = 0;
  std::ifstream myfile ("/Users/holly/Downloads/KITTI/poses/00.txt");
  double x =0, y=0, z = 0;
  double x_prev, y_prev, z_prev;
  if (myfile.is_open())
  {
    while (( std::getline (myfile,line) ) && (i<=frame_id))
    {
      z_prev = z;
      x_prev = x;
      y_prev = y;
      std::istringstream in(line);
      //cout << line << '\n';
      for (int j=0; j<12; j++)  {
        in >> z ;
        if (j==7) y=z;
        if (j==3)  x=z;
      }
      
      i++;
    }
    myfile.close();
  }

  else {
    std::cout << "Unable to open file";
    return 0;
  }

  return sqrt((x-x_prev)*(x-x_prev) + (y-y_prev)*(y-y_prev) + (z-z_prev)*(z-z_prev)) ;

}


void drawFeaturePoints(cv::Mat image, std::vector<cv::Point2f>& points){
    int radius = 2;
    
    for (int i = 0; i < points.size(); i++)
    {
        circle(image, cvPoint(points[i].x, points[i].y), radius, CV_RGB(255,255,255));
    }
}


cv::Mat loadImageLeft(int frame_id, std::string filepath){
    char file[200];
    // sprintf(filename, "/Users/holly/Downloads/KITTI/sequences/00/image_0/%06d.png", frame_id);
    sprintf(file, "/image_0/%06d.png", frame_id);

    std::string filename = filepath + std::string(file);

    cv::Mat image = cv::imread(filename);
    cvtColor(image, image, cv::COLOR_BGR2GRAY);

    return image;
}

cv::Mat loadImageRight(int frame_id, std::string filepath){
    char file[200];
    // sprintf(filename, "/Users/holly/Downloads/KITTI/sequences/00/image_1/%06d.png", frame_id);
    // sprintf(filename, filepath+"/image_1/%06d.png", frame_id);
    sprintf(file, "/image_1/%06d.png", frame_id);

    std::string filename = filepath + std::string(file);

    cv::Mat image = cv::imread(filename);
    cvtColor(image, image, cv::COLOR_BGR2GRAY);

    return image;
}


void initializeImagesFeatures(int current_frame_id, std::string filepath,
                        cv::Mat& image_left_t0, cv::Mat& image_right_t0,
                        FeatureSet& features){

    image_left_t0 = loadImageLeft(current_frame_id, filepath);
    image_right_t0 = loadImageRight(current_frame_id, filepath);

    featureDetectionFast(image_left_t0, features.points);        

    for(int i = 0; i < features.points.size(); i++)
    {
      features.ages.push_back(0);
    }

}

void display(int frame_id, cv::Mat& trajectory, cv::Mat& pose, std::vector<Matrix>& pose_matrix_gt, float fps)
{
    // draw estimated trajectory 
    int x = int(pose.at<double>(0)) + 300;
    int y = int(pose.at<double>(2)) + 100;
    circle(trajectory, cv::Point(x, y) ,1, CV_RGB(255,0,0), 2);

    // draw ground truth trajectory 
    cv::Mat pose_gt = cv::Mat::zeros(1, 3, CV_64F);
    
    pose_gt.at<double>(0) = pose_matrix_gt[frame_id].val[0][3];
    pose_gt.at<double>(1) = pose_matrix_gt[frame_id].val[0][7];
    pose_gt.at<double>(2) = pose_matrix_gt[frame_id].val[0][11];
    x = int(pose_gt.at<double>(0)) + 300;
    y = int(pose_gt.at<double>(2)) + 100;
    circle(trajectory, cv::Point(x, y) ,1, CV_RGB(255,255,0), 2);


    // print info

    // rectangle( traj, Point(10, 30), Point(550, 50), CV_RGB(0,0,0), CV_FILLED);
    // sprintf(text, "FPS: %02f", fps);
    // putText(traj, text, textOrg, fontFace, fontScale, Scalar::all(255), thickness, 8);

    cv::imshow( "Trajectory", trajectory );


    cv::waitKey(1);
}

void integrateOdometryMono(int frame_id, cv::Mat& pose, cv::Mat& Rpose, const cv::Mat& rotation, const cv::Mat& translation_mono)
{
    double scale = 1.00;
    scale = getAbsoluteScale(frame_id);

    std::cout << "translation_mono: " << scale*translation_mono.t() << std::endl;

    if ((scale>0.1)&&(translation_mono.at<double>(2) > translation_mono.at<double>(0)) && (translation_mono.at<double>(2) > translation_mono.at<double>(1))) 
    {
      pose = pose + scale * Rpose * translation_mono;
      Rpose = rotation * Rpose;
    }
    
    else {
     std::cout << "[WARNING] scale below 0.1, or incorrect translation" << std::endl;
    }


}

void integrateOdometryScale(int frame_id, cv::Mat& pose, cv::Mat& Rpose, const cv::Mat& rotation, const cv::Mat& translation_mono, const cv::Mat& translation_stereo)
{

    double scale = sqrt((translation_stereo.at<double>(0))*(translation_stereo.at<double>(0)) 
                        + (translation_stereo.at<double>(1))*(translation_stereo.at<double>(1))
                        + (translation_stereo.at<double>(2))*(translation_stereo.at<double>(2))) ;

    // if (scale<10) {
    if ((scale>0.1)&&(translation_mono.at<double>(2) > translation_mono.at<double>(0)) && (translation_mono.at<double>(2) > translation_mono.at<double>(1))) 
    {
      pose = pose +scale *  Rpose * translation_mono;
      Rpose = rotation * Rpose;

    }

}

void integrateOdometryStereo(int frame_id, cv::Mat& pose, cv::Mat& Rpose, const cv::Mat& rotation, const cv::Mat& translation_stereo)
{

    double scale = sqrt((translation_stereo.at<double>(0))*(translation_stereo.at<double>(0)) 
                        + (translation_stereo.at<double>(1))*(translation_stereo.at<double>(1))
                        + (translation_stereo.at<double>(2))*(translation_stereo.at<double>(2))) ;

    if ((scale>0.1)&&(translation_stereo.at<double>(2) > translation_stereo.at<double>(0)) && (translation_stereo.at<double>(2) > translation_stereo.at<double>(1))) 
    {
      pose = pose + Rpose * translation_stereo;
      Rpose = rotation * Rpose;

    }
    else 
    {
     std::cout << "[WARNING] scale below 0.1, or incorrect translation" << std::endl;
    }

}