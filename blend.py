import os,sys,cv2
import numpy as np
import scipy

def ls_images(d, ext):
	return sorted([os.path.join(d, f) for f in os.listdir(d) if f.endswith(ext)])

def get_bg(I, bg_vals = (0, 255, 0)):
	mask = np.ones(I.shape[:2], np.bool)
	for ch,val in enumerate(bg_vals):
		mask = np.logical_and(I[...,ch] == val, mask)
	mask = np.repeat(mask.reshape(mask.shape + (1,)), repeats = 3, axis = 2)
	return mask

def get_fg(I, bg_vals = (0, 255, 0)):
	mask = get_bg(I, bg_vals)
	Iret = I.copy()
	Iret[mask] = 0
	return Iret

def get_blobs(I):
	unused, Ithresh = cv2.threshold(I, 20, 255, 0)
	contours, hierarchy = cv2.findContours(Ithresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
	min_area = 9
	blobs = []
	for contour in contours:
		area = cv2.contourArea(contour)
		if area < min_area:
			continue
		M = cv2.moments(contour)
		x = int(M['m10']/M['m00'])
		y = int(M['m01']/M['m00'])
		blobs.append((x,y))
	return blobs

def guass_kern(size, sizey = None):
	size = int(size)
	if not sizey:
		sizey = size
	else:
		sizey = int(sizey)
	x,y = scipy.mgrid[-size:size+1, -sizey:sizey+1]
	g = scipy.exp(-(x**2/float(size*8)+y**2/float(sizey*8)))
	return g/g.max()

def gen_heatmap(w, h, pts, k_size):
	Iret = np.zeros((h+k_size*2,w+k_size*2), np.uint)
	Ig = (guass_kern(k_size)*255).astype(np.uint8)
	Ig = Ig[:-1,:-1]
	for (x,y) in pts:
		Iret[y:y+k_size*2, x:x+k_size*2] += Ig
	Iret = np.clip(Iret, 0, 255).astype(np.uint8)

	return Iret[k_size:-k_size, k_size:-k_size]

def get_bbox(pts):
	x1 = pts[0][0]
	y1 = pts[0][1]
	x2 = x1
	y2 = y1
	for p in pts:
		x1 = min(x1, p[0])
		y1 = min(y1, p[1])
		x2 = max(x2, p[0])
		y2 = max(y2, p[1])
	return (x1,y1), (x2,y2)

obj_type = 'airplane'
raw_imgs = ls_images(os.path.join('evaluation_m', obj_type, 'query', 'img'), '.png')
raw_msks = ls_images(os.path.join('evaluation_m', obj_type, 'query', 'mask'), '.png')
prediction_imgs = ls_images(os.path.join('evaluation_m', obj_type, 'prediction'), '.png')
output_dir = os.path.join('evaluation_m', obj_type, 'blend')
if not os.path.exists(output_dir):
	os.mkdir(output_dir)

hand_imgs = []
contact_imgs = []
for raw_img in raw_imgs:
	name = os.path.splitext(os.path.basename(raw_img))[0]
	print name
	hand_img = [f for f in prediction_imgs if os.path.basename(f).startswith(name + '_hand')][0]
	contact_img = [f for f in prediction_imgs if os.path.basename(f).startswith(name + '_contact')][0]
	render_img = [f for f in prediction_imgs if os.path.basename(f).startswith(name + '_render')][0]

	Iraw = cv2.imread(raw_img)

	Icontact = get_fg(cv2.imread(contact_img))
	cv2.imshow('Icontact', Icontact)
	blobs = get_blobs(Icontact[...,2])
	print blobs

	Ih = gen_heatmap(Icontact.shape[1], Icontact.shape[0], blobs, 25)
	Icontact[...,0] = 0
	Icontact[...,1] = 0
	Icontact[...,2] = Ih
	Ialpha_contact = np.repeat(Ih.reshape(Ih.shape + (1,)), 3, 2)/255.0
	I = Iraw * (1 - Ialpha_contact) + Icontact * Ialpha_contact
	I = np.clip(I, 0, 255).astype(np.uint8)

	Ihand = cv2.imread(hand_img)
	
	Ialpha_hand = (~get_bg(Ihand)) * 0.75
	I = I * (1 - Ialpha_hand) + get_fg(Ihand) * Ialpha_hand
	I = np.clip(I, 0, 255).astype(np.uint8)

	cv2.imshow('I', I)

	p1,p2 = get_bbox(blobs)
	Iraw = Iraw * (1 - Ialpha_contact) + Icontact * Ialpha_contact
	Iraw = np.clip(Iraw, 0, 255).astype(np.uint8)	
	cv2.rectangle(Iraw, p1, p2, (255,0,0), 2)
	cv2.imshow('Iraw', Iraw)

	Ipose = cv2.imread(render_img)
	cv2.imshow('Ipose', Ipose)

	cv2.imwrite(os.path.join(output_dir, name + '_1.png'), I)
	cv2.imwrite(os.path.join(output_dir, name + '_2.png'), Iraw)
	cv2.imwrite(os.path.join(output_dir, name + '_3.png'), Ipose)

	if cv2.waitKey(10) & 0xFF == 27:
		sys.exit(0)



