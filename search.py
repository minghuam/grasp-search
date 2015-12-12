import cv2, pickle, sys, os, math
import numpy as np
from scipy.spatial.distance import hamming
import shutil

def ls_files(d, ext):
	return sorted([os.path.join(d, f) for f in os.listdir(d) if f.endswith(ext)])

def get_contour(I):
	Igray = cv2.cvtColor(I, cv2.COLOR_BGR2GRAY)
	unused, Ithresh = cv2.threshold(Igray, 5, 255, 0)
	contours, hierarchy = cv2.findContours(Ithresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
	max_index = -1
	max_area = -1
	for i, contour in enumerate(contours):
		area = cv2.contourArea(contour)
		if max_area < area:
			max_index = i
			max_area = area
	contour = contours[max_index]
	return contour

def calc_distance(I1, I2):
	I1 = cv2.resize(I1, (16, 16)) > 128
	I2 = cv2.resize(I2, (16, 16)) > 128
	I1 = I1.ravel() * 1
	I2 = I2.ravel() * 1
	return hamming(I1, I2)

def get_bg(I, bg_vals = (0,255,0)):
	mask = np.ones(I.shape[:2], np.bool)
	for ch,val in enumerate(bg_vals):
		mask = np.logical_and(I[...,ch] == val, mask)
	return np.repeat(mask.reshape(mask.shape + (1,)), repeats = 3, axis = 2)

def find_closest(mask, hand_dir, obj_dir):
	Is = cv2.imread(mask)
	b_contour = get_contour(Is)
	rect = cv2.boundingRect(b_contour)

	Ib = Is[rect[1]:rect[1]+rect[3], rect[0]:rect[0]+rect[2], 0]
	Ib = cv2.resize(Ib, (512, 512))

	cv2.imshow('Ib', Ib)

	min_img = None
	min_distance = float('inf')
	min_t_contour = None
	min_It = None
	for t in ls_files(obj_dir, '.png'):
		It = cv2.imread(t)
		bg_mask = get_bg(It)
		It[bg_mask] = 0
		It[~bg_mask] = 255

		t_contour = get_contour(It)
		rect = cv2.boundingRect(t_contour)
		It = It[rect[1]:rect[1]+rect[3], rect[0]:rect[0]+rect[2], 1]
		It = cv2.resize(It, (512, 512))

		dist = calc_distance(Ib, It)
		if min_distance > dist:
			min_distance = dist
			min_img = t
			min_t_contour = t_contour
			min_It = It
	cv2.imshow('It', min_It)
	min_hand_img = os.path.join(hand_dir, os.path.basename(min_img))
	min_contact_img = os.path.join(contact_dir, os.path.basename(min_img))
	return min_img, min_hand_img, min_contact_img, b_contour, min_t_contour


obj_type = 'car'

mask_dir = 'evaluation/{}/query/mask'.format(obj_type)
img_dir = 'evaluation/{}/query/img'.format(obj_type)
hand_dir = 'evaluation/{}/db/hand'.format(obj_type)
obj_dir = 'evaluation/{}/db/obj'.format(obj_type)
contact_dir = 'evaluation/{}/db/contact'.format(obj_type)
render_dir = 'evaluation/{}/db/render'.format(obj_type)
output_dir = 'evaluation/{}/prediction'.format(obj_type)

imgs = ls_files(img_dir, '.png')
masks = ls_files(mask_dir, '.png')

for (img, mask) in zip(imgs, masks):
	print img,mask
	I = cv2.imread(img)
	t_img, t_hand_img, t_contact_img, b_contour, t_contour = find_closest(mask, hand_dir, obj_dir)

	M = cv2.moments(b_contour)
	bx = int(M['m10']/M['m00'])
	by = int(M['m01']/M['m00'])
	Sb = cv2.contourArea(b_contour)

	M = cv2.moments(t_contour)
	tx = int(M['m10']/M['m00'])
	ty = int(M['m01']/M['m00'])
	St = cv2.contourArea(t_contour)

	scale = math.sqrt(Sb/float(St))
	Ihand = cv2.imread(t_hand_img)
	Icontact = cv2.imread(t_contact_img)

	It = cv2.imread(t_img)
	Irender = cv2.imread(os.path.join(render_dir, os.path.basename(t_img)))

	h,w = It.shape[:2]
	h = int(h*scale)
	w = int(w*scale)
	Ihand = cv2.resize(Ihand, (w,h))
	It = cv2.resize(It, (w,h))
	Icontact = cv2.resize(Icontact, (w,h))
	Irender = cv2.resize(Irender, (w,h))
	tx = int(tx*scale)
	ty = int(ty*scale)

	It_canvas = np.zeros((It.shape[0] + I.shape[0]*2, It.shape[1] + I.shape[1]*2, 3), np.uint8)
	It_canvas[...,1] = 255
	It_canvas[I.shape[0]:I.shape[0]+It.shape[0], I.shape[1]:I.shape[1]+It.shape[1], :] = It

	Ihand_canvas = np.zeros((It.shape[0] + I.shape[0]*2, It.shape[1] + I.shape[1]*2, 3), np.uint8)
	Ihand_canvas[...,1] = 255
	Ihand_canvas[I.shape[0]:I.shape[0]+It.shape[0], I.shape[1]:I.shape[1]+It.shape[1], :] = Ihand

	Icontact_canvas = np.zeros((It.shape[0] + I.shape[0]*2, It.shape[1] + I.shape[1]*2, 3), np.uint8)
	Icontact_canvas[...,1] = 255
	Icontact_canvas[I.shape[0]:I.shape[0]+It.shape[0], I.shape[1]:I.shape[1]+It.shape[1], :] = Icontact

	Irender_canvas = np.zeros((It.shape[0] + I.shape[0]*2, It.shape[1] + I.shape[1]*2, 3), np.uint8)
	Irender_canvas[...,1] = 255
	Irender_canvas[I.shape[0]:I.shape[0]+It.shape[0], I.shape[1]:I.shape[1]+It.shape[1], :] = Irender

	crop_x = tx - bx + I.shape[1]
	crop_y = ty - by + I.shape[0]
	crop_w = I.shape[1]
	crop_h = I.shape[0]

	It_crop = It_canvas[crop_y:crop_y+crop_h, crop_x:crop_x+crop_w, :]
	Ihand_crop = Ihand_canvas[crop_y:crop_y+crop_h, crop_x:crop_x+crop_w, :]
	Icontact_crop = Icontact_canvas[crop_y:crop_y+crop_h, crop_x:crop_x+crop_w, :]
	Irender_crop = Irender_canvas[crop_y:crop_y+crop_h, crop_x:crop_x+crop_w, :]

	basename = os.path.basename(img)
	name = os.path.splitext(basename)[0]
	cv2.imwrite(os.path.join(output_dir, name + '_1.png'), cv2.imread(os.path.join(render_dir, os.path.basename(t_img))))
	cv2.imwrite(os.path.join(output_dir, name + '_hand.png'), Ihand_crop)
	cv2.imwrite(os.path.join(output_dir, name + '_contact.png'), Icontact_crop)
	cv2.imwrite(os.path.join(output_dir, name + '_render.png'), Irender_crop)

	I = cv2.addWeighted(I, 0.5, Icontact_crop, 0.75, 0)

	cv2.imshow('Iraw', I)
	cv2.imshow('It_crop', It_crop)
	cv2.imshow('Ihand_crop', Ihand_crop)


	if 0xFF & cv2.waitKey(10) == 27:
		sys.exit(0)



