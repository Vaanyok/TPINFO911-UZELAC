    #include <cstdio>
    #include <iostream>
    #include <algorithm>
    #include <opencv2/core/utility.hpp>
    #include <opencv2/imgproc.hpp>
    #include <opencv2/imgcodecs.hpp>
    #include <opencv2/highgui.hpp>

    using namespace cv;
    using namespace std;

    struct ColorDistribution {
        float data[512]; // 8x8x8 = 512 cases
        int nb;

        ColorDistribution() { reset(); }
        ColorDistribution& operator=(const ColorDistribution& other) = default;

        void reset() {
            std::fill(std::begin(data), std::end(data), 0.0f);
            nb = 0;
        }

        void add(Vec3b color) {
            Mat hsv;
            cvtColor(Mat(1, 1, CV_8UC3, &color), hsv, COLOR_BGR2HSV);
            Vec3b hsvColor = hsv.at<Vec3b>(0, 0);

            int h = hsvColor[0] / 32;
            int s = hsvColor[1] / 32;
            int v = hsvColor[2] / 64;

            int index = h * 64 + s * 8 + v;
            data[index] += 1.0f;
            nb++;
        }

        void finished() {
            for (int i = 0; i < 512; i++) {
                data[i] /= nb;
            }
        }

        float distance(const ColorDistribution& other) const {
            float dist = 0.0f;
            for (int i = 0; i < 512; i++) {
                float numerator = (data[i] - other.data[i]) * (data[i] - other.data[i]);
                float denominator = data[i] + other.data[i];
                if (denominator != 0) {
                    dist += numerator / denominator;
                }
            }
            return 0.5f * dist;
        }
    };

    float minDistance(const ColorDistribution& h, const std::vector<ColorDistribution>& hists) {
        float min_dist = FLT_MAX;
        for (const auto& hist : hists) {
            float dist = h.distance(hist);
            if (dist < min_dist) {
                min_dist = dist;
            }
        }
        return min_dist;
    }

    ColorDistribution getColorDistribution(Mat input, Point pt1, Point pt2) {
        ColorDistribution cd;
        for (int y = pt1.y; y < pt2.y; y++)
            for (int x = pt1.x; x < pt2.x; x++)
                cd.add(input.at<Vec3b>(y, x));
        cd.finished();
        return cd;
    }

    void addColorDistributionIfUnique(ColorDistribution& new_cd, vector<ColorDistribution>& col_hists, float threshold) {
        float min_dist = minDistance(new_cd, col_hists);
        if (min_dist > threshold) {
            col_hists.push_back(new_cd);
            cout << "Nouvelle distribution ajoutée (distance minimale = " << min_dist << ")" << endl;
        } else {
            cout << "Distribution ignorée (trop similaire, distance minimale = " << min_dist << ")" << endl;
        }
    }

    float distance_threshold = 0.05;
    void onThresholdChange(int value, void*) {
        distance_threshold = value / 100.0f;
        cout << "Seuil de distance mis à jour : " << distance_threshold << endl;
    }

    Mat recoObject(Mat input, const std::vector<std::vector<ColorDistribution>>& all_col_hists, 
                const std::vector<Vec3b>& colors, int bloc) {
        Mat output = input.clone();
        int height = input.rows;
        int width = input.cols;
        
        for (int y = 0; y <= height - bloc; y += bloc) {
            for (int x = 0; x <= width - bloc; x += bloc) {
                Point pt1(x, y);
                Point pt2(x + bloc, y + bloc);

                ColorDistribution block_cd = getColorDistribution(input, pt1, pt2);
                float min_dist = FLT_MAX;
                int closest_color_index = 0;

                for (int i = 0; i < all_col_hists.size(); i++) {
                    float dist = minDistance(block_cd, all_col_hists[i]);
                    if (dist < min_dist) {
                        min_dist = dist;
                        closest_color_index = i;
                    }
                }

                Vec3b color = colors[closest_color_index];
                for (int y_b = y; y_b < y + bloc; ++y_b) {
                    for (int x_b = x; x_b < x + bloc; ++x_b) {
                        output.at<Vec3b>(y_b, x_b) = color;
                    }
                }
            }
        }
        
        return output;
    }

    int main(int argc, char** argv) {
        Mat img_input;
        VideoCapture* pCap = new VideoCapture(1);
        const int width = 640;
        const int height = 480;
        const int size = 50;

        if (!pCap->isOpened()) {
            cout << "Couldn't open image / camera ";
            return 1;
        }

        pCap->set(CAP_PROP_FRAME_WIDTH, 640);
        pCap->set(CAP_PROP_FRAME_HEIGHT, 480);
        (*pCap) >> img_input;
        if (img_input.empty()) return 1;

        Point pt1(width / 2 - size / 2, height / 2 - size / 2);
        Point pt2(width / 2 + size / 2, height / 2 + size / 2);

        namedWindow("input", 1);
        createTrackbar("Distance seuil", "input", 0, 100, onThresholdChange);
        setTrackbarPos("Distance seuil", "input", 5);
        bool freeze = false;

        vector<vector<ColorDistribution>> all_col_hists;
        vector<ColorDistribution> col_hists_object;
        vector<Vec3b> colors = {Vec3b(0, 0, 0), Vec3b(0, 0, 255)};
        bool recognition_mode = false;

        while (true) {
            char c = (char)waitKey(50);
            if (pCap != nullptr && !freeze)
                (*pCap) >> img_input;
            if (c == 27 || c == 'q')
                break;
            if (c == 'f')
                freeze = !freeze;

            ColorDistribution cd = getColorDistribution(img_input, pt1, pt2);
            cv::rectangle(img_input, pt1, pt2, Scalar(255, 255, 255), 1);
            if (c == 'v')
            {
                Point gh(0, 0);
                Point gb(width / 2, height);
                Point dh(width / 2, 0);
                Point db(width, height);
                ColorDistribution cd_gh = getColorDistribution(img_input, gh, gb);
                ColorDistribution cd_dh = getColorDistribution(img_input, dh, db);
                float dist = cd_gh.distance(cd_dh);
                cout << "Distance : " << dist << endl;
            }

            if (c == 'b') {
                const int bbloc = 128;
                vector<ColorDistribution> col_hists;
                for (int y = 0; y <= height - bbloc; y += bbloc) {
                    for (int x = 0; x <= width - bbloc; x += bbloc) {
                        Point pt1(x, y);
                        Point pt2(x + bbloc, y + bbloc);
                        ColorDistribution block_cd = getColorDistribution(img_input, pt1, pt2);
                        //col_hists.push_back(block_cd);
                        addColorDistributionIfUnique(block_cd, col_hists, distance_threshold);
                    }
                }
                all_col_hists.insert(all_col_hists.begin(), col_hists);
                cout << "Nombre d'histogrammes de fond : " << col_hists.size() << endl;
            }

            if (c == 'a') {
                ColorDistribution part_cd = getColorDistribution(img_input, pt1, pt2);
                col_hists_object.push_back(part_cd);
                cout << "Histogramme ajouté pour l'objet actuel. Total des échantillons : " 
                    << col_hists_object.size() << endl;
            }

            if (c == 'n') {
                if (!col_hists_object.empty()) {
                    all_col_hists.push_back(col_hists_object);
                    colors.push_back(Vec3b(rand() % 256, rand() % 256, rand() % 256));
                    col_hists_object.clear();
                    cout << "Nouvel objet ajouté. Total d'objets : " << all_col_hists.size() - 1 << endl;
                } else {
                    cout << "Pas d'échantillons pour cet objet. Utilisez 'a' pour ajouter des échantillons." << endl;
                }
            }

            if (c == 'r' && !all_col_hists.empty()) {
                recognition_mode = !recognition_mode;
                cout << "Mode reconnaissance : " << (recognition_mode ? "Activé" : "Désactivé") << endl;
            }

            if (recognition_mode) {
                Mat reco = recoObject(img_input, all_col_hists, colors, 16);
                Mat gray;
                cvtColor(img_input, gray, COLOR_BGR2GRAY);
                cvtColor(gray, img_input, COLOR_GRAY2BGR);
                Mat output = 0.5 * reco + 0.5 * img_input;
                imshow("input", output);
            } else {
                imshow("input", img_input);
            }
        }

        return 0;
    }
