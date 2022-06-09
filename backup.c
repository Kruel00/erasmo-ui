
void *write_on_device(void *device_selected){
    int res, dio_tmp;
    int outfd, blocks;

    storage_device_t * device_recived;
    unsigned char *wrkPos;
    unsigned char *fprint;
    //long long skip = 0;
    long long seek = 1;
    static int blk_sz = 512;
    int scsi_cdbsz_out = DEF_SCSI_CDBSZ;
    char inf[512];
    unsigned char *wrkBuff;
    unsigned char *wrkBuff2;    

    device_recived = malloc(sizeof(device_selected));
    device_recived  =  (storage_device_t *)device_selected;


    printf("despues del = device: %s\n", device_recived->sg_name);
    uint8_t erasmosign[] = 
    { 
      0x51, 0x75, 0x61, 0x6E, 0x74, 0x75, 0x6D, 0x20, 0x65, 0x72, 0x61, 0x73, 0x6D, 0x6F, 
      0x28, 0x52, 0x29, 0x20, 0x62, 0x79, 0x20, 0x4D, 0x6F, 0x62, 0x69, 0x6C, 0x65, 0x20, 
      0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x73, 0x20, 0x54, 0x65, 0x61, 0x6D
    };

    strcpy(inf,device_recived->sg_name);

    blocks=128;
    int bpt = 128;
    
    int device_blocks = device_recived->total_sectors;

    size_t psz = getpagesize();
    wrkBuff = malloc(blk_sz * bpt + psz);

    wrkBuff2 = malloc(512);
    long long int tfwide = blk_sz * bpt + psz;
    
    uint8_t data[tfwide];

    memset(data,0x30,sizeof(data));

    wrkPos = wrkBuff;
    memcpy(wrkPos,&data,sizeof(data));
    
    fprint = wrkBuff2;
    memcpy(fprint,&erasmosign,sizeof(erasmosign));

    //open device.
    if ((outfd = sg_cmds_open_device(inf, 1, 0)) < 0)
    {   
        fprintf(stderr, ME " Device %s dont exist\n", inf);
    }

    dio_tmp = 0;

    for(int i = 1;i < (device_blocks / blocks);i++){

        printf("%lli de %i blocks...\n",seek, device_blocks);

        res = sg_write(outfd, wrkPos, blocks, seek, blk_sz, scsi_cdbsz_out, oflag.fua, oflag.dpo, &dio_tmp);
        seek += blocks;
    }

    int blk_remains = device_blocks - seek;

    if(seek > 0){

        for(int i = 1;i < blk_remains + 2 ;i++){
            printf("%lli de %i blocks...\n",seek, device_blocks);
            res = sg_write(outfd, wrkPos, 1, seek, blk_sz, scsi_cdbsz_out, oflag.fua, oflag.dpo, &dio_tmp);
            seek++;
            system("clear");
        }
    }

    printf("restantes: %lli", device_blocks - seek);

    if (res = sg_write(outfd, fprint, 0, 0, blk_sz, scsi_cdbsz_out, oflag.fua, oflag.dpo, &dio_tmp)){
        printf("OK");
    }

    free(wrkBuff);
    free(wrkBuff2);
    
    return 0;
}






    //write code ###########################################################################################################
    int res, dio_tmp;
    int outfd, blocks;

    unsigned char *wrkPos;
    unsigned char *fprint;

    long long seek = 0;
    static int blk_sz = 512;
    int scsi_cdbsz_out = DEF_SCSI_CDBSZ;
    char inf[512];
    unsigned char *wrkBuff;
    unsigned char *wrkBuff2;    

    uint8_t erasmosign[] = 
    { 
      0x51, 0x75, 0x61, 0x6E, 0x74, 0x75, 0x6D, 0x20, 0x65, 0x72, 0x61, 0x73, 0x6D, 0x6F, 
      0x28, 0x52, 0x29, 0x20, 0x62, 0x79, 0x20, 0x4D, 0x6F, 0x62, 0x69, 0x6C, 0x65, 0x20, 
      0x44, 0x65, 0x76, 0x69, 0x63, 0x65, 0x73, 0x20, 0x54, 0x65, 0x61, 0x6D
    };

    strcpy(inf,erasing_device.sg_name);

    blocks=128;
    int bpt = 128;
    
    int device_blocks = erasing_device.total_sectors - 2;

    size_t psz = getpagesize();
    wrkBuff = malloc(blk_sz * bpt + psz);

    wrkBuff2 = malloc(sizeof(erasmosign));
    long long int tfwide = blk_sz * bpt + psz;
    
    uint8_t data[tfwide];

    memset(data,0x30,sizeof(data));

    wrkPos = wrkBuff;
    memcpy(wrkPos,&data,sizeof(data));
    
    fprint = wrkBuff2;
    memcpy(fprint,&erasmosign,sizeof(erasmosign));

    //open device.
    if ((outfd = sg_cmds_open_device(inf, 1, 0)) < 0)
    {   
        fprintf(stderr, ME " Device %s dont exist\n", inf);
    }

    dio_tmp = 0;
    char  *serial_device;
    ulong pocento;
    double progress;
    //char progress_text;
    //bucle de escritura ###################################################################################################

  for(int i = 0;i < (device_blocks / blocks);i++){
        pocento = seek * 100 ;
        progress = pocento / device_blocks;
        printf("%lli de %i sectors...\n",seek, device_blocks);
        printf("porcent: %i%%\n",(int)progress);
        

        //gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_PROGRESS,porca,-1);
        //sprintf(progress_text,"Progress %i%%",(int)porca);
        //gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_PROGRESS_TEXT,progress_text,-1);
        //sprintf(serial_device,"%s",erasing_device.serial);

        res = sg_write(outfd, wrkPos, blocks, seek, blk_sz, scsi_cdbsz_out, oflag.fua, oflag.dpo, &dio_tmp);
        seek += blocks;
    }

    int blk_remains = device_blocks - seek;

    if(seek > 0){

        for(int i = 0;i < blk_remains + 2 ;i++){
            printf("%lli de %i sectors...\n",seek, device_blocks);
            res = sg_write(outfd, wrkPos, 1, seek, blk_sz, scsi_cdbsz_out, oflag.fua, oflag.dpo, &dio_tmp);
            seek++;
        }
    }

    printf("restantes: %lli", device_blocks - seek);

    //if (res = sg_write(outfd, fprint, 1, 0, blk_sz, scsi_cdbsz_out, oflag.fua, oflag.dpo, &dio_tmp)){
    //    printf("OK");
   // }

    free(wrkBuff);
    free(wrkBuff2);

    //end write code ##############################################################################################



    int erase_device()
{
    pthread_t h_eraser;

    storage_device_t device_erasing;
    init_storage_device(&device_erasing);
    
    find_device_by_serial(&device_list,&device_erasing,seleccionado);

    if(pthread_create(&h_eraser, NULL, erase_thread_init ,NULL ) != 0)
        return 1;


    pthread_mutex_destroy(&mutex);

}



void *erase_thread_init(){

    GtkTreeIter storage_device_itr;
    gboolean is_iter_valid  = gtk_tree_model_get_iter_first(model,&storage_device_itr);
    storage_device_t erasing_device;
    init_storage_device(&erasing_device);
    //find_device_by_serial(&device_list, &erasing_device,seleccionado);

    while (is_iter_valid == TRUE)
	{
		gchar *device_serial;
		gtk_tree_model_get(model, &storage_device_itr, STORAGE_DEVICE_SERIAL, &device_serial, -1);

		if (strcmp(seleccionado, device_serial) == 0)
		{
			g_free(device_serial);
            break;
		}
		g_free(device_serial);
		is_iter_valid = gtk_tree_model_iter_next(model, &storage_device_itr);
    }

        gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_STATE_TEXT,"Erasing...",-1);

        gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_PROGRESS_TEXT,"Calculating",-1);

        //write


    gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_PROGRESS_TEXT,EMPTY_STRING,-1);
    
    

    gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_STATE_TEXT,"Erased",-1);
    gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_PROGRESS_TEXT,"Finished",-1);
    gtk_tree_store_set(treestore,&storage_device_itr,ERASING_STORAGE_DEVICE_PROGRESS,(double)100,-1);
    return NULL;
} 



void *erasing_progress(void *dev_data)
{
    GtkTreeIter storage_device_itr;
    gboolean is_iter_valid  = gtk_tree_model_get_iter_first(model,&storage_device_itr);

    storage_device_t erasing_device;
    init_storage_device(&erasing_device);
    //find_device_by_serial(&device_list,&erasing_device,seleccionado);

     while (is_iter_valid == TRUE)
	{
		gchar *device_serial;
		gtk_tree_model_get(model, &storage_device_itr, STORAGE_DEVICE_SERIAL, &device_serial, -1);

		if (strcmp(seleccionado, device_serial) == 0)
		{
			g_free(device_serial);
            break;
		}
		g_free(device_serial);
		is_iter_valid = gtk_tree_model_iter_next(model, &storage_device_itr);
	}

}


