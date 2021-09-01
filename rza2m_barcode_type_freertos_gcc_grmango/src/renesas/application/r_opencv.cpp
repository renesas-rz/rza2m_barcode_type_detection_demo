/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
* Copyright (C) 2018 Renesas Electronics Corporation. All rights reserved.
*******************************************************************************/


#include <opencv.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "r_bcd_main.h"
#include "r_opencv.h"
#include "r_bcd_drp_application.h"

R_Contour	app_contours[2];


#ifdef __cplusplus
}
#endif


using namespace cv;
using namespace std;

#define FLTTOFIX(fracbits, x) \
	int((x) * (float)((int)(1) << (fracbits)))

int cv_findcontours_boundingrect(uint8_t *img, uint16_t width, uint16_t height, uint32_t frameId)
{
	Mat src;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	int i;
	RotatedRect rect;
	int contours_no = 0;
	int area_width, area_height;
	float wh_ratio;
	double area;
	Point2f box[4];
	R_Box r_box;
	R_Rect r_rect;
	int j, outofimage;
	int barcode_square_min = (width/10) * (height/10);

	src = Mat(height,width,CV_8UC1,img);
	findContours( src, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0) );

	for( i = 0; i< contours.size(); i++ )
	{
		if (frameId == 0)
		{
			//get contour of dilate image
			area = contourArea(contours[i]);
			Rect position = boundingRect(contours[i]);
			if (position.x <= 4 || position.y <= 4 ||
				position.x + position.width >= (width - 4) ||
				position.y + position.height >= (height - 4) ||
				abs(position.width - position.height) > ((position.width + position.height) >> 3))
			{
				//filter out the contour on image boundary
				continue;
			}

			if (area > barcode_square_min)// &&
				//abs(position.width - position.height) < ((position.width + position.height) >> 3))
			{
				rect = minAreaRect(contours[i]);
				//Add 4 pixel margin
				rect.size.width += 4;
				rect.size.height += 4;
				rect.points(box);
				outofimage = 0;
				for (j=0; j<4; j++)
				{
					r_box.box[j][0] = (int)box[j].x;
					r_box.box[j][1] = (int)box[j].y;
					if (r_box.box[j][0] < 0 || r_box.box[j][0] >= width || r_box.box[j][1] < 0 || r_box.box[j][1] >= height)
					{
						outofimage = 1;
						break;
					}
				}
				if (outofimage == 0)
				{
					memcpy(&app_contours[frameId].box[contours_no], &r_box, sizeof(R_Box));
					r_rect.x = position.x-4;
					r_rect.y = position.y-4;
					r_rect.width = position.width+8;
					r_rect.height = position.height+8;
					memcpy(&app_contours[frameId].rect[contours_no], &r_rect, sizeof(R_Rect));
					contours_no++;
					if (contours_no >= DRP_R_FINDC_BUFF_RECT_NUM)
						break;
				}
			}
		}
		else
		{
			//this contour has child contour
			Rect position = boundingRect(contours[i]);

			r_rect.x = position.x;
			r_rect.y = position.y;
			r_rect.width = position.width;
			r_rect.height = position.height;
			memcpy(&app_contours[frameId].rect[contours_no], &r_rect, sizeof(R_Rect));
			contours_no++;
		}
	}

	src.release();

	return contours_no;
}

int cv_moments(uint8_t* src, uint16_t width, uint16_t height, uint16_t* center_x, uint16_t* center_y, double* angle)
{
	double vx, vy, area;
	CvMoments m;
	Mat imgSrc = Mat(height,width,CV_8UC1,src);

	m = moments(imgSrc, 1);

	area = m.m00;
	*center_x = m.m10 / area;
	*center_y = m.m01 / area;

	vx = m.mu20 - m.mu02;
	vy = m.mu11 * 2.0;

	if (vy == 0)
	{
		*angle = 0;
	}
	else
	{
		if (vy < 0)
		{
			vx = - vx;
			vy = - vy;
		}
		*angle = atan2(vy, vx) / 2.0;
	}
	return 0;


}
#if 0
static void get_transform_points(R_Box r_box, uint16_t* width, uint16_t* height, Point2f srcTri[3], Point2f dstTri[3])
{

}

void cv_getAffineTransform(R_Box r_box, int32_t m[6], uint16_t* width, uint16_t* height)
{
	Mat warp_mat( 2, 3, CV_32FC1 );
	float mf[6];
	Point2f srcTri[3];
	Point2f dstTri[3];
	int i, left=0, right=0, top=0, bottom=0;
	int left_x, right_x, top_y, bottom_y;
	int left_top, top_right, right_bottom, bottom_left;
	int dx, dy;

	left_x = right_x = r_box.box[0][0];
	top_y = bottom_y = r_box.box[0][1];
	for (i=1;i<4;i++)
	{
		if (left_x > r_box.box[i][0])
		{
			left_x = r_box.box[i][0];
			left = i;
		}
		if (right_x < r_box.box[i][0])
		{
			right_x = r_box.box[i][0];
			right = i;
		}
		if (top_y > r_box.box[i][1])
		{
			top_y = r_box.box[i][1];
			top = i;
		}
		if (bottom_y < r_box.box[i][1])
		{
			bottom_y = r_box.box[i][1];
			bottom = i;
		}
	}

	dx = r_box.box[left][0] - r_box.box[top][0];
	dy = r_box.box[left][1] - r_box.box[top][1];
	left_top = (int)sqrt((double)dx*dx + (double)dy*dy);
	dx = r_box.box[top][0] - r_box.box[right][0];
	dy = r_box.box[top][1] - r_box.box[right][1];
	top_right = (int)sqrt((double)dx*dx + (double)dy*dy);
	dx = r_box.box[right][0] - r_box.box[bottom][0];
	dy = r_box.box[right][1] - r_box.box[bottom][1];
	right_bottom = (int)sqrt((double)dx*dx + (double)dy*dy);
	dx = r_box.box[bottom][0] - r_box.box[left][0];
	dy = r_box.box[bottom][1] - r_box.box[left][1];
	bottom_left = (int)sqrt((double)dx*dx + (double)dy*dy);

	//DRP findcontour requires width and hight are multiple of 8
	if (left_top % 8 != 0)
	{
		left_top = (left_top + 7) & ~7;
	}
	//minimal width of DRP findcontour is 64
	if (left_top < 64)
	{
		left_top = 64;
	}

	if (top_right % 8 != 0)
	{
		top_right = (top_right + 7) & ~7;
	}
	//minimal height of DRP findcontour is 64
	if (top_right < 32)
	{
		top_right = 32;
	}

	srcTri[0].x = (float)r_box.box[left][0]; srcTri[0].y = (float)r_box.box[left][1];
	srcTri[1].x = (float)r_box.box[top][0]; srcTri[1].y = (float)r_box.box[top][1];
	srcTri[2].x = (float)r_box.box[right][0]; srcTri[2].y = (float)r_box.box[right][1];
	dstTri[0].x = 0.f; dstTri[0].y = 0.f;
	dstTri[1].x = (float)left_top; dstTri[1].y = 0.f;
	dstTri[2].x = dstTri[1].x; dstTri[2].y = (float)top_right;
	*width = left_top;
	*height = top_right;
	warp_mat = getAffineTransform( srcTri, dstTri );

	memcpy(mf, warp_mat.data, 6*sizeof(float));
	for (i=0; i<6; i++)
	{
		m[i] = (int32_t)(FLTTOFIX(16,mf[i]));
	}

}
#endif

int cv_affine_transform(uint8_t *src, uint8_t *dst, uint16_t* w, uint16_t* h, R_Box r_box)
{
	Mat img_in, img_out;
	int i, left=0, right=0, top=0, bottom=0;
	int left_x, right_x, top_y, bottom_y;
	int left_top, top_right, right_bottom, bottom_left;
	int dx, dy;
	Point2f srcTri[3];
	Point2f dstTri[3];

	Mat warp_mat( 2, 3, CV_32FC1 );
	img_in = Mat(*h, *w, CV_8UC1, src);


	left_x = right_x = r_box.box[0][0];
	top_y = bottom_y = r_box.box[0][1];
	for (i=1;i<4;i++)
	{
		if (left_x > r_box.box[i][0])
		{
			left_x = r_box.box[i][0];			left = i;
		}
		if (right_x < r_box.box[i][0])
		{
			right_x = r_box.box[i][0];			right = i;
		}
		if (top_y > r_box.box[i][1])
		{
			top_y = r_box.box[i][1];			top = i;
		}
		if (bottom_y < r_box.box[i][1])
		{
			bottom_y = r_box.box[i][1];			bottom = i;
		}
	}

	dx = r_box.box[left][0] - r_box.box[top][0];
	dy = r_box.box[left][1] - r_box.box[top][1];
	left_top = (int)sqrt((double)dx*dx + (double)dy*dy);
	dx = r_box.box[top][0] - r_box.box[right][0];
	dy = r_box.box[top][1] - r_box.box[right][1];
	top_right = (int)sqrt((double)dx*dx + (double)dy*dy);
	dx = r_box.box[right][0] - r_box.box[bottom][0];
	dy = r_box.box[right][1] - r_box.box[bottom][1];
	right_bottom = (int)sqrt((double)dx*dx + (double)dy*dy);
	dx = r_box.box[bottom][0] - r_box.box[left][0];
	dy = r_box.box[bottom][1] - r_box.box[left][1];
	bottom_left = (int)sqrt((double)dx*dx + (double)dy*dy);

	//DRP findcontour requires width and hight are multiple of 8
	//DRP binarization require width is multiple of 32, height is multiple of 8
	if (left_top % 32 != 0)
	{
		left_top = (left_top + 31) & ~31;
	}
	//minimal width of DRP findcontour is 64
	if (left_top < 64)
	{
		left_top = 64;
	}

	if (top_right % 8 != 0)
	{
		top_right = (top_right + 7) & ~7;
	}
	//minimal height of DRP findcontour is 32
	//minimal height of DRP binarization adaptive is 40
	if (top_right < 40)
	{
		top_right = 40;
	}

	srcTri[0].x = (float)r_box.box[left][0]; srcTri[0].y = (float)r_box.box[left][1];
	srcTri[1].x = (float)r_box.box[top][0]; srcTri[1].y = (float)r_box.box[top][1];
	srcTri[2].x = (float)r_box.box[right][0]; srcTri[2].y = (float)r_box.box[right][1];
	dstTri[0].x = 2.0f; dstTri[0].y = 2.0f;
	dstTri[1].x = (float)left_top - 4; dstTri[1].y = 2.0f;
	dstTri[2].x = dstTri[1].x; dstTri[2].y = (float)top_right - 4;
	*w = left_top;
	*h = top_right;
	img_out = Mat(top_right, left_top, CV_8UC1, dst, 0);

	warp_mat = getAffineTransform( srcTri, dstTri );
	warpAffine( img_in, img_out, warp_mat, img_out.size() );
	//threshold(img_out, img_out, 254, 255, CV_THRESH_BINARY);

	img_in.release();
	img_out.release();


	return 0;
}

#if 0
/*
type defiitiobn:
	CV_MOP_ERODE        =0,
   	CV_MOP_DILATE       =1,
	CV_MOP_OPEN         =2,
    CV_MOP_CLOSE        =3,
*/
int cv_morphologyEx(uint8_t *img, uint16_t width, uint16_t height, uint16_t kw, uint16_t kh, int type, uint8_t *out)
{
	Mat element = getStructuringElement(MORPH_RECT, Size(kw, kh) );
	Mat imgSrc, imgDst;

	imgSrc = Mat(height,width,CV_8UC1,img);
	imgDst = Mat(height,width,CV_8UC1,out);

	morphologyEx(imgSrc, imgDst, type, element);

	imgSrc.release();
	imgDst.release();

	//R_CACHE_L1DataInvalidLine((void *)out, width*height);
	R_CACHE_L1DataCleanInvalidAll();

	return 0;
}

int cv_bitwise_not(uint8_t *src, uint16_t in_w, uint16_t in_h)
{
	Mat gray_src;

	gray_src = Mat(in_h,in_w,CV_8UC1,src);

	bitwise_not(gray_src,gray_src);

	gray_src.release();

	return 0;
}

int cv_threshold(uint8_t *src, uint16_t in_w, uint16_t in_h, uint8_t thres, uint8_t mode)
{
	Mat gray_src;

	gray_src = Mat(in_h,in_w,CV_8UC1,src);

	if (mode == 0)
	{
		threshold(gray_src, gray_src, thres, 255, THRESH_BINARY);
	}
	else
	{
		threshold(gray_src, gray_src, 254, 255, THRESH_BINARY | THRESH_OTSU);
	}

	gray_src.release();

	return 0;
}

int cv_crop(uint8_t *src, uint16_t in_w, uint16_t in_h, uint16_t out_x, uint16_t out_y, uint16_t out_w, uint16_t out_h, uint8_t color_channel)
{
	Rect area;
	Mat crop_src, imageROI, img_out;

	if (color_channel == 1)
		crop_src = Mat(in_h,in_w,CV_8UC1,src);
	else if (color_channel == 3)
		crop_src = Mat(in_h,in_w,CV_8UC3,src);
	else
		return -1;

	area.x = out_x;
	area.y = out_y;
	area.width = out_w;
	area.height = out_h;

	imageROI = crop_src(area);
	imageROI.convertTo(img_out, img_out.type());

	memcpy(src, img_out.data, img_out.cols*img_out.rows*color_channel);

	crop_src.release();

	return 0;
}

/* Crop an image from src and extend destination image to specified border*/
static void img_crop_and_makeborder(uint8_t *src, uint8_t *dst, \
						uint16_t src_width, uint16_t src_height, \
						uint16_t offset_x, uint16_t offset_y, \
						uint16_t crop_width, uint16_t crop_height, \
						uint8_t top, uint8_t bottom, uint8_t left, uint8_t right, \
						uint8_t val)
{
	uint8_t *s = (uint8_t*)src;
	uint8_t *d = (uint8_t*)dst;
	uint16_t x,y;
	uint16_t dst_width = crop_width + left + right;



	//fill top border
	for (y = 0; y < top; y++)
	{
		for (x = 0; x < dst_width; x++)
		{
			*d++ = 0;
		}
	}

	for (y = 0; y < crop_height; y++)
	{
		//fill left border
		for (x = 0; x < left; x++)
		{
			*d++ = 0;
		}

		//copy src image to dst
		s = src + (y + offset_y) * src_width + offset_x;
		for (x = 0; x < crop_width; x++)
		{
			*d++ = *s++;
		}

		//fill right border
		for (x = 0; x < right; x++)
		{
			*d++ = 0;
		}
	}

	//fill bottom border
	for (y = 0; y < bottom; y++)
	{
		for (x = 0; x < dst_width; x++)
		{
			*d++ = 0;
		}
	}

}

int cv_crop_copy_make_border_resize(uint8_t *src, uint16_t in_w, uint16_t in_h, uint16_t area_x,
		uint16_t area_y, uint16_t area_w, uint16_t area_h, uint8_t* dst, uint16_t out_w, uint16_t out_h)
{
	Mat src_m, dst_m, crop_m;
	Rect area;
	int top=0, bottom=0, left=0, right=0;
	float ratio_src, ratio_dst;
	uint8_t border_width = 1;

	src_m = Mat(in_h,in_w,CV_8UC1,src);
	dst_m = Mat(out_h,out_w,CV_8UC1,dst);
/*
	area.x = area_x;
	area.y = area_y;
	area.width = area_w;
	area.height = area_h;
	crop_m = src_m(area);

	ratio_src = (float)((area_w*10)/area_h);
	ratio_dst = (float)((out_w*10)/out_h);
	if (ratio_src > ratio_dst)
	{
		left = right = border_width;
		top = bottom = (((int)(area_w * 10 / ratio_dst) - area_h) >> 1) + border_width;
		copyMakeBorder(crop_m, crop_m, top, bottom, left, right, BORDER_CONSTANT, 0);
	}
	else if (ratio_src < ratio_dst)
	{
		top = bottom = border_width;
		left = right = (((int)(area_h * ratio_dst) / 10 - area_w) >> 1) + border_width;
		copyMakeBorder(crop_m, crop_m, top, bottom, left, right, BORDER_CONSTANT, 0);
	}
*/
	ratio_src = (float)((area_w*10)/area_h);
	ratio_dst = (float)((out_w*10)/out_h);
	if (ratio_src > ratio_dst)
	{
		left = right = border_width;
		top = bottom = (((int)(area_w * 10 / ratio_dst) - area_h) >> 1) + border_width;
	}
	else if (ratio_src < ratio_dst)
	{
		top = bottom = border_width;
		left = right = (((int)(area_h * ratio_dst) / 10 - area_w) >> 1) + border_width;
	}
	else
	{
		top = bottom = left = right = border_width;
	}

	crop_m.create(area_h+top+bottom, area_w+left+right, CV_8UC1);
	img_crop_and_makeborder(src, crop_m.data, in_w, in_h, area_x, area_y, area_w, area_h, top, bottom, left, right, 0);

	resize(crop_m, dst_m, dst_m.size(),0,0,INTER_LINEAR ); //INTER_NEAREST
	//erode(dst_m, dst_m, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)) );
	//dilate(dst_m, dst_m, getStructuringElement(MORPH_ELLIPSE, Size(3, 3)) );
	threshold(dst_m,dst_m,127,255,THRESH_BINARY);

	crop_m.release();
	src_m.release();
	dst_m.release();

	return 0;
}
#endif
