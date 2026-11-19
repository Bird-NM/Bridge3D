# Bridge3D

Ứng dụng desktop C++17 / OpenGL 3.3 Core cho đồ án mô hình hóa cầu. Có cầu treo Golden Gate và cầu dây văng sông Hiếu, dựng bằng mesh thể tích từ thông số trong `config/bridges.cfg`.

## Windows / Visual Studio 2022

Cài Visual Studio 2022 với workload **Desktop development with C++**, Windows SDK, CMake ≥3.24, Git và Python 3 (đưa vào PATH để GLAD sinh mã). Card đồ họa cần driver hỗ trợ OpenGL 3.3 Core. Mở Developer PowerShell tại thư mục dự án rồi chạy

```powershell
cmake --preset vs2022
cmake --build --preset release
ctest --preset release
.\build\windows\Release\Bridge3D.exe
```

CMake tải dependency qua HTTPS từ GitHub trong lần configure đầu tiên. Các phiên bản được cố định trong CMakeLists.txt gồm GLFW 3.4, GLM 1.0.1, GLAD 0.1.36, ImGui 1.91.8 và stb theo commit. GLAD dùng đặc tả OpenGL đóng gói, sinh API 3.3 Core. Không dùng immediate mode. Các shader nằm trong `src/renderer.cpp` để executable dùng cùng phiên bản shader với mã nguồn.

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

Nếu display `:99` đã hoạt động, sử dụng display đó. Chế độ smoke kết thúc sau 72 khung hình và xuất sáu ảnh thực tế trong `captures/`. Mỗi lần chạy ghi đè sáu ảnh này. Nút Save PNG ghi tên mới theo thời gian, chụp riêng vùng xem cùng chú thích và trục.

## Thao tác

- Kéo chuột trái để orbit, nhấp để chọn nhóm cấu kiện. Nhấp nền để bỏ chọn. Chuột giữa kéo để pan, cuộn để zoom.
- F căn toàn cầu, R khôi phục camera. 1 phối cảnh tổng thể, 2 chính diện, 3 bên, 4 từ trên, 5 góc dọc theo đường.
- Thanh trên chọn cầu, chế độ mặt/cạnh, góc nhìn, so sánh đồng bộ và xuất PNG.
- Bảng trái bật/tắt, chọn, cô lập, tập trung camera vào nhóm, chỉnh trực giao và cách hiện cạnh/đỉnh. Explode chỉ phục vụ trình bày.
- Bảng phải hiển thị vật liệu, hộp bao, số lượng hình học và vai trò. Đơn vị kích thước là đơn vị mô hình giả định.
- Truck load bật tải, Animate chạy/tạm dừng, Load position chỉnh vị trí. Deform bật minh họa biến dạng, Magnification phóng đại, Original outline chồng hình ban đầu. Reset simulation đưa về trạng thái đầu.

Giao diện dùng tiếng Anh để sử dụng font ImGui mặc định. Tài liệu bằng tiếng Việt. Lựa chọn tác động tới nhóm cấu kiện theo ID.

## Kiểm chứng và giới hạn

Đã biên dịch Release bằng GCC 14.2 trên Debian 13, chạy cửa sổ GLFW với Mesa llvmpipe trong Xvfb, kiểm tra lỗi OpenGL, chọn hình học hiện/ẩn và xuất PNG. CTest kiểm tra hai hệ cầu, chỉ số mesh, pháp tuyến hữu hạn và điều kiện biến dạng bằng không tại gối. Số đo hiệu năng và ảnh của lần chạy cuối ở `docs/validation.md`.

Windows / Visual Studio 2022 có preset và hướng dẫn, chưa được biên dịch hoặc chạy trên Windows trong phiên này. Tương tác bằng người dùng, DPI trên màn hình Windows và hiệu năng GPU thực chưa được xác minh. Smoke tự động kiểm tra render và picking, không thay cho kiểm thử thủ công toàn bộ giao diện.

Hình học suy ra từ ảnh concept. Toàn bộ kích thước, số dây, tiết diện và vị trí neo là giả định dễ sửa. Hai mặt phẳng dây của cầu Hiếu là phương án mô hình hóa lựa chọn từ hình tham chiếu. Hình nền nước phẳng, ánh sáng định hướng, chưa có bóng đổ. Minh họa biến dạng dùng hàm quy ước, không giải cân bằng lực và không tính ứng suất, lực cáp hay hệ số an toàn. Ứng dụng phục vụ học tập và quan sát hình học.

Xem `docs/modeling.md` để biết thuật toán và giả định.
