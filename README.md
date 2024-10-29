## **1. Clone repository**
```
   mkdir <prj_root>
   cd <prj_root>
   git clone -b working_version_0.1 https://github.com/ts-jang-v/vivix-fe.git
```

## **2. i.mx8mm yocto system download**
```
   참조링크: https://github.com/nxp-imx/imx-manifest/tree/imx-linux-nanbield
   cd <prj_root>
   repo init -u https://github.com/nxp-imx/imx-manifest -b imx-linux-nanbield -m imx-6.6.3-1.0.0.xml
   repo sync
```

