
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
