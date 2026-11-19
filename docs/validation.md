# Kết quả kiểm tra

Kiểm tra ngày 19/11/2026 trên Debian 13, GCC 14.2, cấu hình Release. Context OpenGL Core do Mesa 25.0.7 llvmpipe (LLVM 19.1.7) cung cấp. GLFW yêu cầu OpenGL 3.3 Core, driver trả về 4.5 Core, GLAD chỉ nạp API 3.3.

- CMake configure và build thành công. Script cloud được chạy lại thành công, dùng package đã tải và bộ công cụ cục bộ.
- CTest chạy 1 bài kiểm tra cho cả hai mô hình, đạt. Kiểm tra chỉ số tam giác/cạnh, tọa độ hữu hạn, pháp tuyến đơn vị, sự khác biệt của hệ cáp và điều kiện cố định tại gối. Neo tháp của dây văng có trọng số dịch chuyển bằng 0.
- Golden Gate có 41.168 đỉnh, Hiếu có 18.616 đỉnh. Có đỉnh trùng vị trí ở mặt sắc để giữ pháp tuyến riêng.
- Smoke tạo cửa sổ desktop thật trên Xvfb, render 72 khung qua 6 kịch bản và kết thúc với mã 0. Kiểm tra ray picking trúng hình học hiện và bỏ qua toàn bộ nhóm ẩn. Không phát hiện lỗi OpenGL trong vòng render.
- Đã kiểm tra bằng mắt các ảnh surface của cả hai cầu. Đây là ảnh lấy từ framebuffer đang chạy.
- Lần đo 72 khung ở cửa sổ 1600×960 đạt 51,9 FPS trên CPU llvmpipe. Vùng xem 1115×836, so sánh chia đôi vùng đó. Đây là phép smoke ngắn, không phải benchmark 1080p trên GPU Windows.

## Ảnh thực tế

| Kịch bản | Tệp |
| --- | --- |
| Golden Gate khung dây | [golden-wire.png](../captures/golden-wire.png) |
| Golden Gate mặt vật thể | [golden-surface.png](../captures/golden-surface.png) |
| Hiếu khung dây | [hieu-wire.png](../captures/hieu-wire.png) |
| Hiếu mặt vật thể | [hieu-surface.png](../captures/hieu-surface.png) |
| So sánh đồng bộ | [comparison.png](../captures/comparison.png) |
| Minh họa biến dạng, phóng đại ×20 | [deformation.png](../captures/deformation.png) |

## Phần chưa xác minh

Chưa có máy Windows trong phiên này để kiểm tra Visual Studio 2022, driver GPU và thao tác/DPI trên Windows. Chưa kiểm thử bằng tay toàn bộ điều khiển UI. Các preset Windows và hướng dẫn là mã cấu hình được cung cấp, chưa phải kết quả chạy Windows.

Cấu hình cloud đã lưu `install_script` và `start_skill` vào bản nháp. Cần xem lại, lưu và Publish trong Environment settings để phát hành snapshot. Chưa xác minh khôi phục trong task mới. Mã nguồn và ảnh thực tế được bàn giao cùng repository.
