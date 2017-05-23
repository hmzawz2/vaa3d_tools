/* cropped3DImageSeries_plugin.cpp
 * This is a test plugin, you can use it as a demo.
 * 2014-06-30 : by Zhi Zhou
 */
 
#include "v3d_message.h"
#include <vector>
#include "cropped3DImageSeries_plugin.h"
#include "../../../released_plugins/v3d_plugins/istitch/y_imglib.h"
#include "../../../hackathon/zhi/APP2_large_scale/readRawfile_func.h"
#include "../terastitcher/src/core/imagemanager/VirtualVolume.h"
#include <qfile.h>
#include <qtextstream.h>
//#include "dirent.h"
#include <qdir.h>
//#include <fstream>
#include "basic_surf_objs.h"



using namespace std;
using namespace iim;

QStringList importSeriesFileList_addnumbersort(const QString & curFilePath)
{
    QStringList myList;
    myList.clear();

    // get the image files namelist in the directory
    QStringList imgSuffix;
    imgSuffix<<"*.tif"<<"*.raw"<<"*.v3draw"<<"*.lsm"
            <<"*.TIF"<<"*.RAW"<<"*.V3DRAW"<<"*.LSM";

    QDir dir(curFilePath);
    if (!dir.exists())
    {
        qWarning("Cannot find the directory");
        return myList;
    }

    foreach (QString file, dir.entryList(imgSuffix, QDir::Files, QDir::Name))
    {
        myList += QFileInfo(dir, file).absoluteFilePath();
    }

    // print filenames
    foreach (QString qs, myList)  qDebug() << qs;

    return myList;
}

template <class T> bool extract_a_channel(T* data1d, V3DLONG *sz, V3DLONG c, void * &outimg);


Q_EXPORT_PLUGIN2(cropped3DImageSeries, cropped3DImageSeries);

 
QStringList cropped3DImageSeries::menulist() const
{
	return QStringList() 
        <<tr("crop a 3D stack from image series")
        <<tr("crop a 3D stack from a 3D image(v3draw/raw format)")
        <<tr("crop a 3D stack from a 3D image(terafly format)")
        <<tr("about");
}

QStringList cropped3DImageSeries::funclist() const
{
	return QStringList()
        <<tr("crop3DImageSeries")
		<<tr("help")
		<<tr("cropTeraFly");
}

void cropped3DImageSeries::domenu(const QString &menu_name, V3DPluginCallback2 &callback, QWidget *parent)
{
    if (menu_name == tr("crop a 3D stack from image series"))
    {
        QString m_InputfolderName = QFileDialog::getExistingDirectory(parent, QObject::tr("Choose the directory including all images "),
                                              QDir::currentPath(),
                                              QFileDialog::ShowDirsOnly);

        QStringList imgList = importSeriesFileList_addnumbersort(m_InputfolderName);

        Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

        V3DLONG count=0;
        foreach (QString img_str, imgList)
        {
            V3DLONG offset[3];
            offset[0]=0; offset[1]=0; offset[2]=0;

            indexed_t<V3DLONG, REAL> idx_t(offset);

            idx_t.n = count;
            idx_t.ref_n = 0; // init with default values
            idx_t.fn_image = img_str.toStdString();
            idx_t.score = 0;

            vim.tilesList.push_back(idx_t);
            count++;
        }

        int NTILES  = vim.tilesList.size();

        unsigned char * data1d_1st = 0;
        V3DLONG in_sz[4];
        int datatype;

        if (!simple_loadimage_wrapper(callback,const_cast<char *>(vim.tilesList.at(0).fn_image.c_str()), data1d_1st, in_sz, datatype))
        {
            fprintf (stderr, "Error happens in reading the subject file [%0]. Exit. \n",vim.tilesList.at(0).fn_image.c_str());
            return;
        }
        if(data1d_1st) {delete []data1d_1st; data1d_1st = 0;}

        V3DLONG sz[4];
        sz[0] = in_sz[0];
        sz[1] = in_sz[1];
        sz[2] = NTILES;
        sz[3] = in_sz[3];

        CropRegionNavigateDialog dialog(parent, sz);
        if (dialog.exec()!=QDialog::Accepted)
            return;

        dialog.update();

        V3DLONG im_cropped_sz[4];
        im_cropped_sz[0] = dialog.xe - dialog.xs + 1;
        im_cropped_sz[1] = dialog.ye - dialog.ys + 1;
        im_cropped_sz[2] = dialog.ze - dialog.zs + 1;
        if(dialog.c == -1)
            im_cropped_sz[3] = in_sz[3];
        else
            im_cropped_sz[3] = 1;

        unsigned char *im_cropped = 0;
        V3DLONG pagesz = im_cropped_sz[0]* im_cropped_sz[1]* im_cropped_sz[2]*im_cropped_sz[3];
        try {im_cropped = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return;}

        V3DLONG i = 0;
        for(V3DLONG ii = dialog.zs; ii <= dialog.ze; ii++)
        {
            unsigned char * data1d = 0;
            V3DLONG in_sz[4];
            int datatype;

            if (!simple_loadimage_wrapper(callback,const_cast<char *>(vim.tilesList.at(ii).fn_image.c_str()), data1d, in_sz, datatype))
            {
                fprintf (stderr, "Error happens in reading the subject file [%0]. Exit. \n",vim.tilesList.at(ii).fn_image.c_str());
                return;
            }
            V3DLONG offsetc;
            if(dialog.c != -1)
            {
                offsetc = dialog.c * in_sz[0] *in_sz[1];
                for(V3DLONG iy = dialog.ys; iy <= dialog.ye; iy++)
                {
                    V3DLONG offsetj = iy*in_sz[0];
                    for(V3DLONG ix = dialog.xs; ix <= dialog.xe; ix++)
                    {
                        im_cropped[i] = data1d[offsetc + offsetj + ix];
                        i++;
                    }
                }
            }
            else
            {
                V3DLONG offsetk_cropped = (ii - dialog.zs) *im_cropped_sz[0] * im_cropped_sz[1];
                for(V3DLONG ic = 0; ic < in_sz[3]; ic++)
                {
                    offsetc = ic * in_sz[0] *in_sz[1] * in_sz[2];
                    V3DLONG offsetc_cropped = ic * im_cropped_sz[0] * im_cropped_sz[1] * im_cropped_sz[2];
                    for(V3DLONG iy = dialog.ys; iy <= dialog.ye; iy++)
                    {
                        V3DLONG offsetj = iy*in_sz[0];
                        V3DLONG offsetj_cropped = (iy - dialog.ys) *im_cropped_sz[0];
                        for(V3DLONG ix = dialog.xs; ix <= dialog.xe; ix++)
                        {
                            im_cropped[offsetc_cropped + offsetk_cropped + offsetj_cropped + ix - dialog.xs] = data1d[offsetc + offsetj + ix];
                        }
                    }
                }
            }

            if(data1d) {delete []data1d; data1d = 0;}
        }

        ImagePixelType pixeltype;
        switch (datatype)
        {
            case 1: pixeltype = V3D_UINT8; break;
            case 2: pixeltype = V3D_UINT16; break;
            case 4: pixeltype = V3D_FLOAT32;break;
            default: v3d_msg("Invalid data type. Do nothing."); return;
        }

        Image4DSimple * new4DImage = new Image4DSimple();
        new4DImage->setData((unsigned char *)im_cropped, im_cropped_sz[0], im_cropped_sz[1], im_cropped_sz[2], im_cropped_sz[3], pixeltype);
        v3dhandle newwin = callback.newImageWindow();
        callback.setImage(newwin, new4DImage);
        callback.setImageName(newwin, "Cropped 3D image");
        callback.updateImageWindow(newwin);

	}
    else if (menu_name == tr("crop a 3D stack from a 3D image(v3draw/raw format)"))
    {
        QString m_InputfolderName =  QFileDialog::getOpenFileName(0, QObject::tr("Open Raw File"),
                                                                  "",
                                                                  QObject::tr("Supported file (*.raw *.RAW *.V3DRAW *.v3draw)"));
        unsigned char * datald = 0;
        V3DLONG *in_zz = 0;
        V3DLONG *in_sz = 0;

        int datatype;

        if (!loadRawRegion(const_cast<char *>(m_InputfolderName.toStdString().c_str()), datald, in_zz, in_sz,datatype,0,0,0,1,1,1))
        {
            return;
        }

        if(datald) {delete []datald; datald = 0;}
        CropRegionNavigateDialog dialog(parent, in_zz);
        if (dialog.exec()!=QDialog::Accepted)
            return;
        dialog.update();

        unsigned char * cropped_image = 0;

        if (!loadRawRegion(const_cast<char *>(m_InputfolderName.toStdString().c_str()), cropped_image, in_zz, in_sz,datatype,dialog.xs,dialog.ys,dialog.zs,dialog.xe+1,dialog.ye+1,dialog.ze+1))
        {
            return;
        }

        ImagePixelType pixeltype;
        switch (datatype)
        {
            case 1: pixeltype = V3D_UINT8; break;
            case 2: pixeltype = V3D_UINT16; break;
            case 4: pixeltype = V3D_FLOAT32;break;
            default: v3d_msg("Invalid data type. Do nothing."); return;
        }

        Image4DSimple * new4DImage = new Image4DSimple();

        void* cropped_image_1channel = 0;
        if(dialog.c !=-1 && in_sz[3] > 1)
        {
            switch(datatype)
            {
                case 1: extract_a_channel(cropped_image, in_sz, dialog.c, cropped_image_1channel); break;
                case 2: extract_a_channel((unsigned short int *)cropped_image, in_sz, dialog.c, cropped_image_1channel); break;
                case 4: extract_a_channel((float *)cropped_image, in_sz, dialog.c, cropped_image_1channel); break;
                default:
                v3d_msg("Right now this plugin supports only UINT8/UINT16/FLOAT32 data. Do nothing."); return;
            }
            new4DImage->createImage(in_sz[0], in_sz[1], in_sz[2], 1, pixeltype);
            memcpy(new4DImage->getRawData(), (unsigned char *)cropped_image_1channel, new4DImage->getTotalBytes());
        }
        else
        {
            new4DImage->createImage(in_sz[0], in_sz[1], in_sz[2], in_sz[3], pixeltype);
            memcpy(new4DImage->getRawData(), (unsigned char *)cropped_image, new4DImage->getTotalBytes());
        }

        v3dhandle newwin = callback.newImageWindow();
        callback.setImage(newwin, new4DImage);
        callback.setImageName(newwin, "Cropped 3D image");
        callback.updateImageWindow(newwin);
        return;
    }
    else if (menu_name == tr("crop a 3D stack from a 3D image(terafly format)"))
    {
        QString m_InputfolderName = QFileDialog::getExistingDirectory(parent, QObject::tr("Choose the directory including the highest terafly images "),
                                                                      QDir::currentPath(),
                                                                      QFileDialog::ShowDirsOnly);
        VirtualVolume* data1d = VirtualVolume::instance(m_InputfolderName.toStdString().c_str());
        V3DLONG in_zz[4];
        in_zz[0] = data1d->getDIM_H();
        in_zz[1] = data1d->getDIM_V();
        in_zz[2] = data1d->getDIM_D();
        in_zz[3] = data1d->getDIM_C();

        CropRegionNavigateDialog dialog(parent, in_zz);
        if (dialog.exec()!=QDialog::Accepted)
            return;
        dialog.update();

        unsigned char * cropped_image = 0;
        cropped_image = data1d->loadSubvolume_to_UINT8(dialog.ys,dialog.ye+1,dialog.xs,dialog.xe+1,dialog.zs,dialog.ze+1);
        V3DLONG in_sz[4];

        in_sz[0] = dialog.xe - dialog.xs + 1;
        in_sz[1] = dialog.ye - dialog.ys + 1;
        in_sz[2] = dialog.ze - dialog.zs + 1;
        in_sz[3] = data1d->getDIM_C();

        Image4DSimple * new4DImage = new Image4DSimple();
        new4DImage->createImage(in_sz[0], in_sz[1], in_sz[2], in_sz[3], V3D_UINT8);
        memcpy(new4DImage->getRawData(), (unsigned char *)cropped_image, new4DImage->getTotalBytes());
        v3dhandle newwin = callback.newImageWindow();
        callback.setImage(newwin, new4DImage);
        callback.setImageName(newwin, "Cropped 3D image");
        callback.updateImageWindow(newwin);
        return;
    }
    else
	{
        v3d_msg(tr("This is plugin to generate 3D stack from image series,"
			"Developed by Zhi Zhou, 2014-06-30"));
	}
}

bool cropped3DImageSeries::dofunc(const QString & func_name, const V3DPluginArgList & input, V3DPluginArgList & output, V3DPluginCallback2 & callback,  QWidget * parent)
{
    if (func_name == tr("crop3d_imageseries"))
	{
        cout<<"Welcome to crop3DImageSeries plugin"<<endl;
        if (output.size() != 1) return false;
        char * inimg_file = ((vector<char*> *)(input.at(0).p))->at(0);
        char * outimg_file = ((vector<char*> *)(output.at(0).p))->at(0);
        long xs, ys, zs, xe, ye, ze;
        int c;
        if (input.size() >= 2)
        {
            vector<char*> paras = (*(vector<char*> *)(input.at(1).p));
            if(paras.size() < 7)
            {
                cout<<"Do not have enought input paremeters"<<endl;
                return false;
            }
            cout<<paras.size()<<endl;
            if(paras.size() >= 1) xs = atoi(paras.at(0)) - 1;
            if(paras.size() >= 2) xe = atoi(paras.at(1)) - 1;
            if(paras.size() >= 3) ys = atoi(paras.at(2)) - 1;
            if(paras.size() >= 4) ye = atoi(paras.at(3)) - 1;
            if(paras.size() >= 5) zs = atoi(paras.at(4)) - 1;
            if(paras.size() >= 6) ze = atoi(paras.at(5)) - 1;
            if(paras.size() >= 7) c = atoi(paras.at(6)) - 1;
         }
        else
        {
            cout<<"Do not have enought input paremeters"<<endl;
            return false;
        }

        cout<<"xs = "<<xs+1<<"; xe = "<<xe+1<<endl;
        cout<<"ys = "<<ys+1<<"; ye = "<<ye+1<<endl;
        cout<<"zs = "<<zs+1<<"; ze = "<<ze+1<<endl;
        cout<<"ch = "<<c+1<<endl;
        cout<<"inimg_folder = "<<inimg_file<<endl;
        cout<<"outimg_file = "<<outimg_file<<endl;

        QString m_InputfolderName(inimg_file);
        QStringList imgList = importSeriesFileList_addnumbersort(m_InputfolderName);

        Y_VIM<REAL, V3DLONG, indexed_t<V3DLONG, REAL>, LUT<V3DLONG> > vim;

        V3DLONG count=0;
        foreach (QString img_str, imgList)
        {
            V3DLONG offset[3];
            offset[0]=0; offset[1]=0; offset[2]=0;

            indexed_t<V3DLONG, REAL> idx_t(offset);

            idx_t.n = count;
            idx_t.ref_n = 0; // init with default values
            idx_t.fn_image = img_str.toStdString();
            idx_t.score = 0;

            vim.tilesList.push_back(idx_t);
            count++;
        }

        int NTILES  = vim.tilesList.size();
        unsigned char * data1d_1st = 0;
        V3DLONG in_sz[4];
        int datatype;

        if (!simple_loadimage_wrapper(callback,const_cast<char *>(vim.tilesList.at(0).fn_image.c_str()), data1d_1st, in_sz, datatype))
        {
            fprintf (stderr, "Error happens in reading the subject file [%0]. Exit. \n",vim.tilesList.at(0).fn_image.c_str());
            return false;
        }
        if(data1d_1st) {delete []data1d_1st; data1d_1st = 0;}

        V3DLONG sz[4];
        sz[0] = in_sz[0];
        sz[1] = in_sz[1];
        sz[2] = NTILES;
        sz[3] = in_sz[3];


        V3DLONG im_cropped_sz[4];
        im_cropped_sz[0] = xe - xs + 1;
        im_cropped_sz[1] = ye - ys + 1;
        im_cropped_sz[2] = ze - zs + 1;
        if(c == -1)
            im_cropped_sz[3] = in_sz[3];
        else
            im_cropped_sz[3] = 1;

        unsigned char *im_cropped = 0;
        V3DLONG pagesz = im_cropped_sz[0]* im_cropped_sz[1]* im_cropped_sz[2]*im_cropped_sz[3];
        try {im_cropped = new unsigned char [pagesz];}
        catch(...)  {v3d_msg("cannot allocate memory for image_mip."); return false;}

        V3DLONG i = 0;
        for(V3DLONG ii = zs; ii <= ze; ii++)
        {
            unsigned char * data1d = 0;
            V3DLONG in_sz[4];
            int datatype;

            if (!simple_loadimage_wrapper(callback,const_cast<char *>(vim.tilesList.at(ii).fn_image.c_str()), data1d, in_sz, datatype))
            {
                fprintf (stderr, "Error happens in reading the subject file [%0]. Exit. \n",vim.tilesList.at(ii).fn_image.c_str());
                return false;
            }
            V3DLONG offsetc;
            if(c != -1)
            {
                offsetc = c * in_sz[0] *in_sz[1];
                for(V3DLONG iy =  ys; iy <=  ye; iy++)
                {
                    V3DLONG offsetj = iy*in_sz[0];
                    for(V3DLONG ix =  xs; ix <=  xe; ix++)
                    {
                        im_cropped[i] = data1d[offsetc + offsetj + ix];
                        i++;
                    }
                }
            }
            else
            {
                V3DLONG offsetk_cropped = (ii -  zs) *im_cropped_sz[0] * im_cropped_sz[1];
                for(V3DLONG ic = 0; ic < in_sz[3]; ic++)
                {
                    offsetc = ic * in_sz[0] *in_sz[1] * in_sz[2];
                    V3DLONG offsetc_cropped = ic * im_cropped_sz[0] * im_cropped_sz[1] * im_cropped_sz[2];
                    for(V3DLONG iy =  ys; iy <=  ye; iy++)
                    {
                        V3DLONG offsetj = iy*in_sz[0];
                        V3DLONG offsetj_cropped = (iy -  ys) *im_cropped_sz[0];
                        for(V3DLONG ix =  xs; ix <=  xe; ix++)
                        {
                            im_cropped[offsetc_cropped + offsetk_cropped + offsetj_cropped + ix -  xs] = data1d[offsetc + offsetj + ix];
                        }
                    }
                }
            }

            if(data1d) {delete []data1d; data1d = 0;}
        }

        simple_saveimage_wrapper(callback, outimg_file,(unsigned char *)im_cropped,im_cropped_sz,datatype);
        if(im_cropped) {delete []im_cropped; im_cropped = 0;}
        return true;
    }
    else if (func_name == "crop3d_raw")
    {
        cout<<"Welcome to crop3DImageSeries plugin"<<endl;
        if (output.size() != 1) return false;
        char * inimg_file = ((vector<char*> *)(input.at(0).p))->at(0);
        char * outimg_file = ((vector<char*> *)(output.at(0).p))->at(0);
        long xs, ys, zs, xe, ye, ze;
        int c;
        if (input.size() >= 2)
        {
            vector<char*> paras = (*(vector<char*> *)(input.at(1).p));
            if(paras.size() < 7)
            {
                cout<<"Do not have enought input paremeters"<<endl;
                return false;
            }
            cout<<paras.size()<<endl;
            if(paras.size() >= 1) xs = atoi(paras.at(0)) - 1;
            if(paras.size() >= 2) xe = atoi(paras.at(1)) - 1;
            if(paras.size() >= 3) ys = atoi(paras.at(2)) - 1;
            if(paras.size() >= 4) ye = atoi(paras.at(3)) - 1;
            if(paras.size() >= 5) zs = atoi(paras.at(4)) - 1;
            if(paras.size() >= 6) ze = atoi(paras.at(5)) - 1;
            if(paras.size() >= 7) c = atoi(paras.at(6)) - 1;
         }
        else
        {
            cout<<"Do not have enought input paremeters"<<endl;
            return false;
        }

        cout<<"xs = "<<xs+1<<"; xe = "<<xe+1<<endl;
        cout<<"ys = "<<ys+1<<"; ye = "<<ye+1<<endl;
        cout<<"zs = "<<zs+1<<"; ze = "<<ze+1<<endl;
        cout<<"ch = "<<c+1<<endl;
        cout<<"inimg_folder = "<<inimg_file<<endl;
        cout<<"outimg_file = "<<outimg_file<<endl;


        unsigned char * datald = 0;
        V3DLONG *in_zz = 0;
        V3DLONG *in_sz = 0;

        int datatype;

        if (!loadRawRegion(inimg_file, datald, in_zz, in_sz,datatype,0,0,0,1,1,1))
        {
            return false;
        }

        if(datald) {delete []datald; datald = 0;}
        unsigned char * cropped_image = 0;

        if (!loadRawRegion(inimg_file, cropped_image, in_zz, in_sz,datatype,xs,ys,zs,xe+1,ye+1,ze+1))
        {
            return false;
        }

        if(c !=-1 && in_sz[3] > 1)
        {
            void* cropped_image_1channel = 0;
            switch(datatype)
            {
                case 1: extract_a_channel(cropped_image, in_sz, c, cropped_image_1channel); break;
                case 2: extract_a_channel((unsigned short int *)cropped_image, in_sz, c, cropped_image_1channel); break;
                case 4: extract_a_channel((float *)cropped_image, in_sz, c, cropped_image_1channel); break;
                default:
                v3d_msg("Right now this plugin supports only UINT8/UINT16/FLOAT32 data. Do nothing."); return false;
            }

            in_sz[3] = 1;
            simple_saveimage_wrapper(callback, outimg_file,(unsigned char *)cropped_image_1channel,in_sz,datatype);
            if(cropped_image) {delete []cropped_image; cropped_image = 0;}
            if(cropped_image_1channel) {delete []cropped_image_1channel; cropped_image_1channel = 0;}
        }
        else
        {
            simple_saveimage_wrapper(callback, outimg_file,(unsigned char *)cropped_image,in_sz,datatype);
            if(cropped_image) {delete []cropped_image; cropped_image = 0;}
        }
    }
	else if (func_name == tr("cropTerafly"))
	{
		vector<char*> infiles, inparas, outfiles;
		if(input.size() >= 1) infiles = *((vector<char*> *)input.at(0).p);
		if(input.size() >= 2) inparas = *((vector<char*> *)input.at(1).p);
		if(output.size() >= 1) outfiles = *((vector<char*> *)output.at(0).p);
		QString length1 = inparas.at(0);
		QString length2 = inparas.at(1);
		QString length3 = inparas.at(2);
		QString m_InputfolderName = infiles.at(0);
		QString inputListFile = infiles.at(1);
		QString savePath = infiles.at(2);
		
		VirtualVolume* data1d = VirtualVolume::instance(m_InputfolderName.toStdString().c_str());
		V3DLONG in_zz[4];
		in_zz[0] = data1d->getDIM_H();
		in_zz[1] = data1d->getDIM_V();
		in_zz[2] = data1d->getDIM_D();
		in_zz[3] = data1d->getDIM_C();

		long cubeSideLength1 = length1.toLong();
		long cubeSideLength2 = length2.toLong();
		long cubeSideLength3 = length3.toLong();
		V3DLONG in_sz[4];
		in_sz[0] = cubeSideLength1;
		in_sz[1] = cubeSideLength2;
		in_sz[2] = cubeSideLength3;
		in_sz[3] = in_zz[3];

		QFile inputFile(inputListFile);
		QTextStream in(&inputFile);
		int count = 0;
		if (inputFile.open(QIODevice::ReadOnly))
		{
			while (!in.atEnd())
			{
				QString line = in.readLine();
				QList<QString> coords = line.split(",");
				if (coords[0] == "##n") continue;
				coords[4] = coords[4].remove(0, 1);
				qDebug() << coords[4] << " " << coords[5] << " " << coords[6];

				QList<QString> zC = coords[4].split(".");
				QList<QString> xC = coords[5].split(".");
				QList<QString> yC = coords[6].split(".");

				long zcenter = zC[0].toLong() - 1;
				long xcenter = xC[0].toLong() - 1;
				long ycenter = yC[0].toLong() - 1;
				cout << zcenter << " " << xcenter << " " << ycenter << endl;

				unsigned char * cropped_image = 0;
				cropped_image = data1d->loadSubvolume_to_UINT8((ycenter-cubeSideLength2/2), (ycenter+cubeSideLength2/2), 
															   (xcenter-cubeSideLength1/2), (xcenter+cubeSideLength1/2), 
															   (zcenter-cubeSideLength3/2), (zcenter+cubeSideLength3/2));
				Image4DSimple* new4DImage = new Image4DSimple();
				new4DImage->createImage(in_sz[0], in_sz[1], in_sz[2], in_sz[3], V3D_UINT8);
				QString saveName = savePath + "/" + coords[5] + "_" + coords[6] + "_" + coords[4] + ".v3draw";
				const char* fileName = saveName.toAscii();

				simple_saveimage_wrapper(callback, fileName, cropped_image, in_sz, 1);
			}
		}
		inputFile.close();

        return true;
	}
	/*else if (func_name == tr("markerCube_apoWhole"))
	{
		QDir dir("Y:/10soma_volumes/3large/");
		QFileInfoList files = dir.entryInfoList();
		for (QList<QFileInfo>::iterator fileIt=files.begin(); fileIt!=files.end(); ++fileIt)
		{
			QString fileName = fileIt->fileName();
			//qDebug() << fileName;
			if (fileName == "." || fileName == "..") 
			{
				fileName.clear();
				continue;	
			}

			QList<QString> fileSplit;
			fileSplit = fileName.split("_");
			QList<QString> aa = fileSplit[2].split(".v3draw");
			double xOld = fileSplit[0].toDouble();
			double yOld = fileSplit[1].toDouble();
			double zOld = aa[0].toDouble();
			//cout << xOld << " " << yOld << " " << zOld << endl;

			fileName = "Y:/10soma_volumes/3large/" + fileName;
			QFile inputFile(fileName);
			if (inputFile.open(QIODevice::ReadOnly))
			{
				QTextStream in(&inputFile);
				while (!in.atEnd())
				{
					QString line = in.readLine();
					QList<QString> lineSplit = line.split(",");
					if (lineSplit[0] == "##x") continue;
					double cubeX = lineSplit[0].toDouble();
					double cubeY = lineSplit[1].toDouble();
					double cubeZ = lineSplit[2].toDouble();

					double distX = cubeX - 200;
					double distY = cubeY - 200;
					double distZ = cubeZ - 200;

					double newX = xOld + distX;
					double newY = yOld + distY;
					double newZ = zOld + distZ;

					cout << newX << " " << newY << " " << newZ << endl;

					CellAPO node;
					node.x = newX;
					node.y = newY;
					node.z = newZ;
					QList<CellAPO> listCell;
					listCell.push_back(node);
					QString nameX = QString::number(newX);
					QString nameY = QString::number(newY);
					QString nameZ = QString::number(newZ);
					QString saveName = "Y:/10soma_volumes/3large/" + nameX + "_" + nameY + "_" + nameZ + ".apo";
					//qDebug() << saveName;
					writeAPO_file(saveName, listCell);
				}
				inputFile.close();
			}
		}

		return true;
	}*/
    else if (func_name == tr("help"))
    {
        cout<<"Usage : v3d -x dllname -f crop3d_imageseries -i <inimg_folder> -o <outimg_file> -p <xs> <xe> <ys> <ye> <zs> <ze> <ch>"<<endl;
        cout<<endl;
        cout<<"xs      minimum in x dimension, start from 1    "<<endl;
        cout<<"xe      maximum in x dimension, start from 1    "<<endl;
        cout<<"ys      minimum in y dimension, start from 1    "<<endl;
        cout<<"ye      maximum in y dimension, start from 1    "<<endl;
        cout<<"zs      minimum in z dimension, start from 1    "<<endl;
        cout<<"ze      maximum in z dimension, start from 1    "<<endl;
        cout<<"ch      the channel value,start from 1 (0 for all channels).  "<<endl;
        cout<<endl;


        cout<<"Usage : v3d -x dllname -f crop3d_raw -i <inimg_folder> -o <outimg_file> -p <xs> <xe> <ys> <ye> <zs> <ze> <ch>"<<endl;
        cout<<endl;
        cout<<"xs      minimum in x dimension, start from 1    "<<endl;
        cout<<"xe      maximum in x dimension, start from 1    "<<endl;
        cout<<"ys      minimum in y dimension, start from 1    "<<endl;
        cout<<"ye      maximum in y dimension, start from 1    "<<endl;
        cout<<"zs      minimum in z dimension, start from 1    "<<endl;
        cout<<"ze      maximum in z dimension, start from 1    "<<endl;
        cout<<"ch      the channel value,start from 1 (0 for all channels).  "<<endl;
        cout<<endl;
		
		cout << "--------------------------------------------------------------" << endl;
		cout << "cropTerafly: This function allows cropping image volumes out of terafly images based on user specified locations. " 
			 << "The input requires a .apo file in which all seed points supposed to be centered in the cropped image cubes are stored."
			 << "\n\nUsage : v3d -x dllname -f cropTerafly -i <teraFly image folder> <the list file that has soma locations> <save folder of cropped images> -p <cube x side lenth> <cube y side length> <cube z side length>" << endl;
        cout << endl;
        cout << "All folders and files specification require full path input.\n\n";
	}
    else
        return false;

	return true;
}

template <class T> bool extract_a_channel(T* data1d, V3DLONG *sz, V3DLONG c, void* &outimg)
{
    if (!data1d || !sz || c<0 || c>=sz[3])
    {
        printf("problem: c=[%ld] sz=[%p] szc=[%ld], data1d=[%p]\n",c, sz, sz[3], data1d);
        return false;
    }

    outimg = (void *) (data1d + c*sz[0]*sz[1]*sz[2]);

//    printf("ok c=[%ld]\n",c);
    return true;
}

