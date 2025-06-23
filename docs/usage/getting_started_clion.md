# Developing using Clion

0. Install Docker and clion

1. Configure the docker image:
```bash
cd firmware/.devcontainer && docker build -f Dockerfile -t sensorhub-clion-docker .
```

2. open the firmware folder in clion

3. Select Toolchain -> Manage toolchains
![alt text](artifacts/guide_clion_toolchain_setup.png)

4. Click the `+` button in the top-left corner and select docker
![alt text](artifacts/guide_clion_toolchain_menu_select.png)

5. Configure it like this (Enter the image on sensorhub-clion-docker:latest and ninja as build tool)
![alt text](artifacts/guide_clion_docker.png)

6. You're set, let it configure and hit the build button afterwards