

/**
 * compile: g++ -std=c++17 MergeMetrics.cc
 */
#include <vector>
#include <map>
#include <cstring>
#include <filesystem>
#include <fstream>

using namespace std;
using std::filesystem::directory_iterator;

/* operator, (leak power (nW), dyn energy (e-15J), delay (ps), area (um^2)) */
map<const char *, double *> opMetrics;

vector<const char *> diffOps;

void removeMetrics(const char *op) {
  for (auto &opMetricsIt : opMetrics) {
    if (!strcmp(opMetricsIt.first, op)) {
      opMetrics.erase(opMetricsIt.first);
      return;
    }
  }
  printf("We could not find %s when trying to delete it!!!\n", op);
}

void updateMetrics(const char *op, double *metric) {
  for (auto &opMetricsIt : opMetrics) {
    if (!strcmp(opMetricsIt.first, op)) {
      double *oldMetric = opMetricsIt.second;
      if ((oldMetric[0] != metric[0])
          || (oldMetric[1] != metric[1])
          || (oldMetric[2] != metric[2])
          || (oldMetric[3] != metric[3])) {
        bool exist = false;
        for (auto diffOp : diffOps) {
          if (!strcmp(diffOp, op)) {
            exist = true;
            break;
          }
        }
        if (!exist) {
          diffOps.push_back(op);
        }
      }
      return;
    }
  }
  opMetrics.insert(std::make_pair(op, metric));
}

int main(int argc, char **argv) {
  string path = argv[1];
  for (const auto &file : directory_iterator(path)) {
    string file_path = file.path().string();
    printf("Read metric file %s\n", file_path.c_str());
    std::ifstream metricFp(file_path);
    std::string line;
    while (std::getline(metricFp, line)) {
      std::istringstream iss(line);
      char *instance = new char[204800];
      int metricCount = -1;
      auto metric = new double[4];
      bool emptyLine = true;
      do {
        std::string numStr;
        iss >> numStr;
        if (!numStr.empty()) {
          emptyLine = false;
          if (metricCount >= 0) {
            metric[metricCount] = std::strtod(numStr.c_str(), nullptr);
          } else {
            sprintf(instance, "%s", numStr.c_str());
          }
          metricCount++;
        }
      } while (iss);
      if (!emptyLine && (metricCount != 4)) {
        printf("%s has %d metrics!\n", file_path.c_str(), metricCount);
        exit(-1);
      }
      updateMetrics(instance, metric);
    }
  }
  printf("diffOps:\n");
  for (auto diffOp : diffOps) {
    printf("%s\n", diffOp);
  }
  for (auto diffOp : diffOps) {
    removeMetrics(diffOp);
  }
  std::ofstream metricFp;
  metricFp.open("fluid.metrics", std::ios_base::app);
  for (auto &opMetricsIt : opMetrics) {
    const char *opName = opMetricsIt.first;
    double *metric = opMetricsIt.second;
    metricFp << opName << "  " << metric[0] << "  " << metric[1] << "  "
             << metric[2] << "  " << metric[3] << "\n";
  }
  metricFp.close();
  return 1;
}
