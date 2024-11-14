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
               const std::vector<Vec3b>& colors, int bloc, int groupSize) {
    Mat output = input.clone();
    int height = input.rows;
    int width = input.cols;
    
    // Matrice pour marquer les labels des régions
    Mat labels = Mat::zeros(height, width, CV_32S);

    // Parcours des blocs, regroupés en groupes de 4x4 blocs
    for (int y = 0; y <= height - bloc; y += bloc) {
        for (int x = 0; x <= width - bloc; x += bloc) {
            Point pt1(x, y);
            Point pt2(x + bloc, y + bloc);

            ColorDistribution block_cd = getColorDistribution(input, pt1, pt2);
            float min_dist = FLT_MAX;
            int closest_color_index = 0;

            // Trouver le label le plus proche pour ce bloc
            for (int i = 0; i < all_col_hists.size(); i++) {
                float dist = minDistance(block_cd, all_col_hists[i]);
                if (dist < min_dist) {
                    min_dist = dist;
                    closest_color_index = i;
                }
            }

            // Attribuer le label du groupe à tous les pixels de ce bloc
            Vec3b color = colors[closest_color_index];
            for (int y_b = y; y_b < y + bloc; ++y_b) {
                for (int x_b = x; x_b < x + bloc; ++x_b) {
                    labels.at<int>(y_b, x_b) = closest_color_index;
                    output.at<Vec3b>(y_b, x_b) = color;
                }
            }
        }
    }

    // Appliquer Watershed pour la segmentation fine des objets (seulement sur les objets)
    Mat markers = Mat::zeros(input.size(), CV_32S);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (labels.at<int>(y, x) == 0) { // Fond
                markers.at<int>(y, x) = -1;  // Fond marqué
            } else {
                markers.at<int>(y, x) = labels.at<int>(y, x); // Objet marqué
            }
        }
    }

    // Application de Watershed pour séparer les objets
    watershed(input, markers);

    // Appliquer une couleur pour visualiser les frontières
    Mat output_watershed = output.clone();
    output_watershed.setTo(Scalar(0, 0, 255), markers == -1); // Frontières en rouge

    // Affichage de l'image avec watershed
    return output_watershed;
}


void displayObjectName(Mat& image, const Vec3b& color, const string& objectName, int x, int y) {
    // Convertir la couleur en format BGR
    Scalar textColor = Scalar(255, 255, 255);  // Blanc pour le texte
    Scalar boxColor = Scalar(0, 0, 0);  // Couleur de fond de la boîte, noire
    
    // Texte à afficher : nom de l'objet
    string colorText = objectName;

    // Calculer la taille du texte pour ajuster la boîte
    int baseLine = 0;
    Size textSize = getTextSize(colorText, FONT_HERSHEY_SIMPLEX, 0.4, 1, &baseLine);
    
    // Dessiner un rectangle pour la boîte autour du texte
    Rect box(x, y, textSize.width + 40, textSize.height + 10);  // Un peu de marge autour du texte
    rectangle(image, box, boxColor, FILLED);  // Dessiner le rectangle rempli
    
    // Ajouter le texte à l'intérieur de la boîte
    putText(image, colorText, Point(x + 40, y + textSize.height + 5), FONT_HERSHEY_SIMPLEX, 0.4, textColor, 1);

    // Ajouter une petite boîte colorée à gauche du texte
    Rect colorBox(x + 10, y + 5, 20, 20);  // Petite boîte à côté du texte
    rectangle(image, colorBox, Scalar(color[0], color[1], color[2]), FILLED);  // Boîte colorée
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
                    // Ajouter l'objet à la liste des histogrammes de tous les objets
                    all_col_hists.push_back(col_hists_object);
                    
                    // Générer une couleur aléatoire pour l'objet
                    Vec3b newColor(rand() % 256, rand() % 256, rand() % 256);
                    colors.push_back(newColor);
                    
                    // Générer un nom d'objet (ici, on l'appelle simplement "Objet X")
                    string objectName = "Objet " + to_string(all_col_hists.size());
                    
                    // Effacer les histogrammes de l'objet actuel
                    col_hists_object.clear();
                    
                    cout << "Nouvel objet ajouté. Total d'objets : " << all_col_hists.size() - 1 << endl;
                } else {
                    cout << "Pas d'échantillons pour cet objet. Utilisez 'a' pour ajouter des échantillons." << endl;
                }
            }

            // Afficher tous les objets ajoutés sur chaque frame
            int y_offset = 10;  // Position initiale pour le premier objet
            for (int i = 0; i < all_col_hists.size(); ++i) {
                // Afficher chaque objet avec sa couleur et son nom
                displayObjectName(img_input, colors[i], "Objet " + to_string(i + 1), 10, y_offset);
                y_offset += 30;  // Espacer les objets affichés
            }


            if (c == 'r' && !all_col_hists.empty()) {
                recognition_mode = !recognition_mode;
                cout << "Mode reconnaissance : " << (recognition_mode ? "Activé" : "Désactivé") << endl;
            }

            if (recognition_mode) {
                Mat reco = recoObject(img_input, all_col_hists, colors, 16, 4);  // Bloc de 16x16, regroupé en 4x4
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
