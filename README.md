# Bridge3D

Ứng dụng desktop C++17 / OpenGL 3.3 Core cho đồ án mô hình hóa cầu. Có cầu treo Golden Gate và cầu dây văng sông Hiếu, dựng bằng mesh thể tích từ thông số trong `config/bridges.cfg`.

## Windows / VS Code với MinGW

Dùng terminal PowerShell trong VS Code tại thư mục chứa `CMakeLists.txt`. Cần CMake ≥3.24, Git, Python 3 và bộ MSYS2/MinGW có `gcc`, `g++`, `mingw32-make` trong PATH.

```powershell
cmake --preset mingw
cmake --build --preset mingw-release
ctest --preset mingw-release
.\build\mingw\Bridge3D.exe
```

Preset dùng thư mục `build/mingw`, tương thích với quy trình cấu hình MinGW bằng lệnh đã hướng dẫn. Sau khi cập nhật mã bằng `git pull`, chạy lại bước build. Cấu hình Windows đang chờ xác minh trên máy Windows.

## Windows / Visual Studio 2022

Cài Visual Studio 2022 với workload **Desktop development with C++**, Windows SDK, CMake ≥3.24, Git và Python 3 (đưa vào PATH để GLAD sinh mã). Card đồ họa cần driver hỗ trợ OpenGL 3.3 Core. Mở Developer PowerShell tại thư mục dự án rồi chạy

```powershell
cmake --preset vs2022
cmake --build --preset release
ctest --preset release
.\build\windows\Release\Bridge3D.exe
```

CMake tải dependency qua HTTPS từ GitHub trong lần configure đầu tiên. Các phiên bản được cố định trong CMakeLists.txt gồm GLFW 3.4, GLM 1.0.1, GLAD 0.1.36, ImGui 1.91.8 và stb theo commit. GLAD dùng đặc tả OpenGL đóng gói, sinh API 3.3 Core. Không dùng immediate mode. Các shader nằm trong `shaders/`. CMake gắn đường dẫn thư mục dự án để ứng dụng đọc cấu hình và shader khi khởi động. Giữ thư mục mã nguồn cùng các tài nguyên tại vị trí đã build.

## Linux

Trên Debian/Ubuntu có quyền quản trị, cài `g++ cmake git python3 libgl-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`. Sau đó

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j 4
ctest --test-dir build --output-on-failure
./build/Bridge3D
```

Trong máy cloud hiện tại đã chuẩn bị công cụ cục bộ ở `/workspace/tooling` vì thư mục hệ thống chỉ đọc. Chạy lại quy trình đã kiểm tra bằng `bash scripts/cloud-setup.sh`. Máy không có màn hình có thể kiểm tra render bằng

```sh
/workspace/tooling/sysroot/usr/bin/Xvfb :99 -screen 0 1600x960x24 -nolisten tcp -ac &
DISPLAY=:99 MESA_SHADER_CACHE_DIR=/tmp/bridge-mesa ./build/Bridge3D --smoke
```

Nếu display `:99` đã hoạt động, sử dụng display đó. Chế độ smoke kết thúc sau 324 khung hình và xuất 27 ảnh thực tế trong `captures/`. Mỗi lần chạy ghi đè 27 ảnh này. Nút Save PNG ghi tên mới theo thời gian, chụp riêng vùng xem cùng chú thích và trục.

## Thao tác

- Kéo chuột trái để orbit, nhấp để chọn nhóm cấu kiện. Nhấp nền để bỏ chọn. Chuột giữa kéo để pan, cuộn để zoom.
- F căn toàn cầu, R khôi phục camera. 1 phối cảnh tổng thể, 2 chính diện, 3 bên, 4 từ trên, 5 góc dọc theo đường.
- Thanh trên chọn cầu, chế độ mặt/cạnh, góc nhìn, so sánh đồng bộ và xuất PNG.
- Bảng trái bật/tắt, chọn, cô lập, tập trung camera vào nhóm, chỉnh trực giao và cách hiện cạnh/đỉnh. Explode phục vụ trình bày.
- Presentation bật vật liệu, trời, bóng, nước và cảnh quan. Inspection đưa về nền trung tính và ẩn cảnh quan. Chế độ mặt/khung dây vẫn chọn từ thanh trên.
- Environment mở cây cảnh quan với hai bờ, đường dẫn, đường dạo, cây, thân cây, nhà, mái, cửa và nước. Có công tắc tổng, làm mờ, gợn nước và khung dây cảnh quan. Mố và khối neo giữ trong nhóm kết cấu.
- Fit [F] vừa khung cầu cùng một đoạn tiếp cận ngắn. Fit whole scene bao hai đường dẫn và cảnh quan gần. Hai lệnh loại mặt nước và địa hình hậu cảnh xa khỏi phép căn khung.
- Các góc Whole scene, Bridge end A/B, Along approach, Scene top nằm trong danh sách góc nhìn. Tower close-up và Road level nằm ở bảng trái.
- Clearance cutaway cắt phần tháp/dây phía trên vùng thông xe để kiểm tra vị trí hai chân từ trên. Chú thích xuất hiện trực tiếp trên vùng xem.
- Bảng phải hiển thị vật liệu, hộp bao, số lượng hình học và vai trò. Đơn vị kích thước là đơn vị mô hình giả định.
- Truck load bật tải, Animate chạy/tạm dừng, Load position chỉnh vị trí. Deform bật minh họa biến dạng, Magnification phóng đại, Original outline chồng hình ban đầu. Reset simulation đưa về trạng thái đầu.

Giao diện dùng tiếng Anh để sử dụng font ImGui mặc định. Tài liệu bằng tiếng Việt. Lựa chọn tác động tới nhóm cấu kiện theo ID.

## Kiểm chứng và giới hạn

Đã biên dịch Release bằng GCC 14.2 trên Debian 13, chạy cửa sổ GLFW với Mesa llvmpipe trong Xvfb, kiểm tra lỗi OpenGL, chọn hình học hiện/ẩn và xuất PNG. CTest kiểm tra hai hệ cầu, chỉ số mesh, pháp tuyến hữu hạn và điều kiện biến dạng bằng không tại gối. Số đo hiệu năng và ảnh của lần chạy cuối ở `docs/validation.md`.

Windows / Visual Studio 2022 có preset và hướng dẫn, chưa được biên dịch hoặc chạy trên Windows trong phiên này. Tương tác bằng người dùng, DPI trên màn hình Windows và hiệu năng GPU thực chưa được xác minh. Smoke tự động kiểm tra render và picking, không thay cho kiểm thử thủ công toàn bộ giao diện.

Hình học suy ra từ ảnh concept. Toàn bộ kích thước, số dây, tiết diện và vị trí neo là giả định dễ sửa. Hai mặt phẳng dây của cầu Hiếu là phương án mô hình hóa lựa chọn từ hình tham chiếu. Cảnh quan sinh mesh thủ tục, mặt nước dùng Fresnel và normal gợn nhẹ, ánh sáng mặt trời có shadow map PCF. MSAA dùng framebuffer trung gian 4 mẫu khi phần cứng hỗ trợ. Minh họa biến dạng dùng hàm quy ước, không giải cân bằng lực và không tính ứng suất, lực cáp hay hệ số an toàn. Ứng dụng phục vụ học tập và quan sát hình học.

Xem [mô hình hóa](docs/modeling.md), [cảnh quan và thông số](docs/environment.md), [pipeline đồ họa và ảnh trước/sau](docs/graphics.md), cùng [kết quả kiểm tra](docs/validation.md).

Các lệnh kiểm tra render bổ sung có thể chạy với cùng `DISPLAY` như lệnh smoke trên Linux. Trên Windows, dùng đường dẫn executable tương ứng.

```sh
./build/Bridge3D --benchmark-golden
./build/Bridge3D --benchmark-hieu
./build/Bridge3D --resize-check
```

Benchmark chạy 120 khung, bỏ 20 khung đầu khỏi thống kê và đồng bộ GPU để đo thời gian hoàn thành. Resize check đổi kích thước cửa sổ rồi thoát sau 70 khung.
