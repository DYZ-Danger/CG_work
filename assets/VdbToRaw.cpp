#include <openvdb/openvdb.h>
#include <openvdb/io/File.h>
#include <openvdb/tools/Dense.h>
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::cout << "Usage: vdb_to_raw input.vdb output.raw" << std::endl;
        return 0;
    }

    const char* inputVDB  = argv[1];
    const char* outputRAW = argv[2];

    openvdb::initialize();

    // 打开 VDB 文件
    openvdb::io::File file(inputVDB);
    file.open();

    // 读取 density grid
    openvdb::GridBase::Ptr baseGrid;
    for (auto nameIter = file.beginName(); nameIter != file.endName(); ++nameIter) {
        if (nameIter.gridName() == "density") {
            baseGrid = file.readGrid(nameIter.gridName());
            break;
        }
    }

    if (!baseGrid) {
        std::cerr << "Density grid not found!" << std::endl;
        return -1;
    }

    auto grid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);

    // 获取 voxel bounding box
    openvdb::CoordBBox bbox = grid->evalActiveVoxelBoundingBox();
    openvdb::Coord dim = bbox.dim();
    int nx = dim.x();
    int ny = dim.y();
    int nz = dim.z();

    // 直接导出VDB原始分辨率，无需采样
    std::cout << "Exporting RAW: " << nx << " x " << ny << " x " << nz << std::endl;
    std::vector<float> buffer(nx * ny * nz, 0.0f);
    for (int z = 0; z < nz; ++z) {
        for (int y = 0; y < ny; ++y) {
            for (int x = 0; x < nx; ++x) {
                float v = grid->tree().getValue(openvdb::Coord(x + bbox.min().x(), y + bbox.min().y(), z + bbox.min().z()));
                buffer[x + y * nx + z * nx * ny] = v;
            }
        }
    }
    // 写 float32 RAW (Z-major)
    std::ofstream out(outputRAW, std::ios::binary);
    if (!out) {
        std::cerr << "Cannot open output file" << std::endl;
        return -1;
    }
    for (int z = 0; z < nz; ++z)
        for (int y = 0; y < ny; ++y)
            for (int x = 0; x < nx; ++x)
            {
                float v = buffer[x + y * nx + z * nx * ny];
                out.write(reinterpret_cast<char*>(&v), sizeof(float));
            }
    out.close();

    // 写meta文件（分辨率）
    std::string metaPath = std::string(outputRAW) + ".meta";
    std::ofstream metaOut(metaPath);
    if (metaOut) {
        metaOut << nx << " " << ny << " " << nz << std::endl;
        metaOut.close();
    } else {
        std::cerr << "Cannot write meta file: " << metaPath << std::endl;
    }

    file.close();
    std::cout << "Done." << std::endl;
    return 0;
}
