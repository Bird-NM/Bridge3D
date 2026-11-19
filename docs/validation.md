# Kết quả kiểm tra

Kiểm tra ngày 19/11/2026 trên Debian 13, GCC 14.2, cấu hình Release. Máy được cấp 5 CPU logic AMD EPYC 9V74. Renderer thực tế là Mesa 25.0.7 llvmpipe, LLVM 19.1.7, chạy trên CPU trong Xvfb. GLFW yêu cầu OpenGL 3.3 Core, context trả về 4.5 Core và GLAD chỉ nạp API 3.3.

## Build và kiểm tra hình học

CMake configure/build Release và CTest thành công. CTest có một bài tổng hợp chạy cả hai cầu cùng một cấu hình Hiếu thay đổi bề rộng, thông xe và vị trí tháp. Các kiểm tra đã chạy gồm

- Chỉ số tam giác/cạnh, tọa độ hữu hạn, pháp tuyến đơn vị, diện tích tam giác và winding khớp pháp tuyến.
- Hai hệ cáp riêng, số lượng điểm neo, neo dầm đúng cao độ và dầm mép dọc, neo Hiếu đúng tâm chân tháp, dây treo Golden Gate đứng.
- Gối có dịch chuyển bằng 0, các đầu neo tháp của dây Hiếu giữ mobility=0.
- SAT tam giác/hộp trên toàn thể tích thông xe và hai lề, cùng 61 vị trí hộp bao xe chạy qua tháp. Lặp lại với carriagewayWidth=10, sidewalkWidth=2, vehicleClearanceHeight=7, hieuTowerX=12.
- Đường tiếp cận khớp vị trí, tiếp tuyến và bề rộng đầu cầu.
- Tam giác địa hình nằm dưới các đỉnh mặt trên của đường/lề, kiểm tra hàm cao độ dọc đường, cây và nhà ngoài vùng nhịp/cáp.
- Sinh lại môi trường từ cùng seed cho cùng tọa độ đỉnh. Mesh cảnh quan có winding/pháp tuyến phù hợp.

## Render và tương tác được tự động kiểm tra

`--smoke` chạy 324 khung qua 27 kịch bản, xuất 27 PNG mới và thoát với mã 0. Cửa sổ thật GLFW dùng Xvfb. Vòng render kiểm tra lỗi OpenGL, thử chọn geometry đang hiện và bỏ qua nhóm ẩn. Kiểm tra cutaway xác nhận ray chọn được mặt đường qua vùng tháp trên cao đã bị cắt.

Framebuffer cửa sổ báo 0 mẫu MSAA. Framebuffer màu trung gian được xác nhận có 4 mẫu MSAA, với RGBA16F và depth24, resolve sang texture HDR rồi tone map. Shadow map 2048×2048 có PCF 3×3. Các framebuffer đều được kiểm tra trạng thái hoàn chỉnh.

`--resize-check` chạy 70 khung, đổi từ 1600×960 sang 1440×900 rồi trở về 1600×960, cấp lại framebuffer và kết thúc với mã 0. Kiểm tra này được chạy trên Linux/Xvfb.

Các ảnh surface, cận đường, cận mố, tháp Hiếu và Inspection đã được xem trực tiếp. PNG được đọc lại để kiểm tra định dạng. Cặp bật/tắt bóng dùng cùng camera và thời gian nước, có khác biệt pixel do bóng. Hai ảnh trước nâng cấp đồ họa được giữ riêng. Các cặp trước/sau và thông số camera ở [graphics.md](graphics.md).

## Hiệu năng đo được

Cửa sổ và framebuffer 1600×960, vùng xem 1115×836. Các số liệu của renderer CPU nằm trong bảng dưới. Benchmark chạy 120 khung, bỏ 20 khung đầu, gọi glFinish để đo thời gian hoàn thành. Chỉ một phiên ứng dụng được chạy trong lúc đo.

| Phép chạy | FPS trung bình |
| --- | --- |
| Golden Gate Presentation | 10,66 |
| Hiếu Presentation | 12,49 |
| Smoke 27 kịch bản | 11,02 |

FPS ở chế độ Presentation trên CPU chưa đạt mục tiêu tương tác mượt. Có công tắc Sun shadows, Water ripples, ẩn nhóm cảnh quan và Inspection để giảm chi phí. Hiệu năng GPU Windows và kiểm tra 1080p trên GPU đang chờ xác minh.

## Bộ ảnh bàn giao

- Bốn góc bắt buộc của tháp Hiếu gồm [dọc đường](../captures/hieu-road-clearance.png), [từ trên với cutaway](../captures/hieu-top-clearance.png), [phối cảnh khung dây](../captures/hieu-pylon-wire.png), [phối cảnh mặt](../captures/hieu-pylon-surface.png). Có thêm [góc ngang](../captures/hieu-pylon-side.png).
- Hai bố cục cảnh quan gồm [Golden Gate](../captures/golden-landscape.png) và [Hiếu](../captures/hieu-landscape.png).
- Bốn đầu cầu, đối chiếu trước/sau, góc camera mới, cận tháp/mặt đường, Inspection, chia đôi và biến dạng nằm trong bảng ảnh ở [graphics.md](graphics.md).

## Phần chưa xác minh và giới hạn

Preset MinGW/MSVC và hướng dẫn VS Code được cung cấp, việc biên dịch và chạy thực tế trên Windows chưa được xác minh trong máy Linux này. Tương tác thủ công toàn bộ giao diện và DPI trên màn hình Windows chưa được kiểm thử. Các bài chạy tự động xác nhận những hành vi cụ thể đã liệt kê.

Cây, nhà, đá và vật liệu dùng mô hình thủ tục với chi tiết giới hạn. SSAO, IBL và planar reflection là các phần chưa triển khai. Kích thước và địa hình là giả định. Biến dạng là minh họa động học phục vụ học tập.

Cấu hình cloud lưu hướng dẫn cài đặt và khởi động vào bản nháp. Publication và khôi phục trong task mới chưa được xác minh. Mã nguồn, shader, cấu hình, tài liệu và ảnh được bàn giao cùng repository.
