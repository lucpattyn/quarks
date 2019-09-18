#ifdef _USE_PLUGINS

#include <filter.hpp>
#include <base64.hpp>

#include <opencv2/opencv.hpp>

#include <algorithm>

using namespace Core;

std::string Filter::gaussian(std::string &image_data) {

    auto image_data_vector = std::vector<char>(image_data.begin(), image_data.end());
    auto image = cv::imdecode(image_data_vector, cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
    auto target_image = image.clone();

    cv::GaussianBlur(image, target_image, cv::Size(5,5), 0, 0);

    std::vector<uchar> buffer;
    cv::imencode(".png", target_image, buffer);

    auto buffer_string = std::string(buffer.begin(), buffer.end());
    auto buffer_base64 = Helpers::Base64::Encode(buffer_string);

    return  buffer_string;
}

void imadjust(const cv::Mat1b& src, cv::Mat1b& dst, int tol = 1, cv::Vec2i in = cv::Vec2i(0, 255), cv::Vec2i out = cv::Vec2i(0, 255))
{
    // src : input CV_8UC1 image
    // dst : output CV_8UC1 imge
    // tol : tolerance, from 0 to 100.
    // in  : src image bounds
    // out : dst image buonds
    
    dst = src.clone();
    
    tol = fmax(0, fmin(100, tol));
    
    if (tol > 0)
    {
        // Compute in and out limits
        
        // Histogram
        std::vector<int> hist(256, 0);
        for (int r = 0; r < src.rows; ++r) {
            for (int c = 0; c < src.cols; ++c) {
                hist[src(r,c)]++;
            }
        }
        
        // Cumulative histogram
        std::vector<int> cum = hist;
        for (size_t i = 1; i < hist.size(); ++i) {
            cum[i] = cum[i - 1] + hist[i];
        }
        
        // Compute bounds
        int total = src.rows * src.cols;
        int low_bound = total * tol / 100;
        int upp_bound = total * (100-tol) / 100;
        in[0] = distance(cum.begin(), lower_bound(cum.begin(), cum.end(), low_bound));
        in[1] = distance(cum.begin(), lower_bound(cum.begin(), cum.end(), upp_bound));
        
    }
    
    // Stretching
    float scale = float(out[1] - out[0]) / float(in[1] - in[0]);
    for (int r = 0; r < dst.rows; ++r)
    {
        for (int c = 0; c < dst.cols; ++c)
        {
            int vs = fmax(src(r, c) - in[0], 0);
            int vd = fmin(int(vs * scale + 0.5f) + out[0], out[1]);
            dst(r, c) = cv::saturate_cast<uchar>(vd);
        }
    }
}


std::string Filter::adjust(std::string &image_data) {
    
    auto image_data_vector = std::vector<char>(image_data.begin(), image_data.end());
    cv::Mat1b image = cv::imdecode(image_data_vector, cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
    auto target_image = image.clone();
    
    imadjust(image, target_image);
    
    std::vector<uchar> buffer;
    cv::imencode(".png", target_image, buffer);
    
    auto buffer_string = std::string(buffer.begin(), buffer.end());
    auto buffer_base64 = Helpers::Base64::Encode(buffer_string);
    
    return  buffer_string;
    

}

void imclahe(cv::Mat& src_image, cv::Mat& image_clahe){
    
    // READ RGB color image and convert it to Lab
    cv::Mat& bgr_image = src_image;
    cv::Mat lab_image;
    cv::cvtColor(bgr_image, lab_image, cv::COLOR_BGR2Lab);
    
    // Extract the L channel
    std::vector<cv::Mat> lab_planes(3);
    cv::split(lab_image, lab_planes);  // now we have the L image in lab_planes[0]
    
    // apply the CLAHE algorithm to the L channel
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(4);
    cv::Mat dst;
    clahe->apply(lab_planes[0], dst);
    
    // Merge the the color planes back into an Lab image
    dst.copyTo(lab_planes[0]);
    cv::merge(lab_planes, lab_image);
    
    // convert back to RGB
    //cv::Mat image_clahe;
    cv::cvtColor(lab_image, image_clahe, cv::COLOR_Lab2BGR);
    
    // display the results  (you might also want to see lab_planes[0] before and after).
    //cv::imshow("image original", bgr_image);
    //cv::imshow("image CLAHE", image_clahe);
    //cv::waitKey();
    
}

std::string Filter::clahe(std::string & image_data){
    auto image_data_vector = std::vector<char>(image_data.begin(), image_data.end());
    auto image = cv::imdecode(image_data_vector, cv::IMREAD_ANYCOLOR | cv::IMREAD_ANYDEPTH);
    //auto target_image = image.clone();
    cv::Mat target_image;
    imclahe(image, target_image);
    
    std::vector<uchar> buffer;
    cv::imencode(".png", target_image, buffer);
    
    auto buffer_string = std::string(buffer.begin(), buffer.end());
    auto buffer_base64 = Helpers::Base64::Encode(buffer_string);
    
    return  buffer_string;
}

#endif
