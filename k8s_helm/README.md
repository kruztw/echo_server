# Install tcp echo server with Helm

## Environment

```shell=
# helm
version.BuildInfo{Version:"v3.6.3", GitCommit:"d506314abfb5d21419df8c7e7e68012379db2354", GitTreeState:"clean", GoVersion:"go1.16.5"}

# minikube
minikube version: v1.25.0
commit: 3edf4801f38f3916c9ff96af4284df905a347c86

# kubectl 
Client Version: version.Info{Major:"1", Minor:"23", GitVersion:"v1.23.1", GitCommit:"86ec240af8cbd1b60bcc4c03c20da9b98005b92e", GitTreeState:"clean", BuildDate:"2021-12-16T11:41:01Z", GoVersion:"go1.17.5", Compiler:"gc", Platform:"linux/amd64"}
Server Version: version.Info{Major:"1", Minor:"23", GitVersion:"v1.23.1", GitCommit:"86ec240af8cbd1b60bcc4c03c20da9b98005b92e", GitTreeState:"clean", BuildDate:"2021-12-16T11:34:54Z", GoVersion:"go1.17.5", Compiler:"gc", Platform:"linux/amd64"}
```

## Installing NGINX Ingress using Helm


`Install helm 3`

```shell=
curl https://raw.githubusercontent.com/helm/helm/master/scripts/get-helm-3 | bash
```

`Add NGINX Ingress repo`

```shell=
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
```

`Install NGINX Ingress on kube-system namespace`

```shell=
helm install -n kube-system ingress-nginx ingress-nginx/ingress-nginx
```

## Edit NGINX Ingress Controller Deployment

```shell=
kubectl edit deployments -n kube-system ingress-nginx-controller

    spec:
      containers:
      - args:
        - /nginx-ingress-controller
        - --publish-service=kube-system/ingress-nginx-controller
        - --election-id=ingress-controller-leader
        - --ingress-class=nginx
        - --configmap=kube-system/ingress-nginx-controller
        - --tcp-services-configmap=$(POD_NAMESPACE)/tcp-services
        - --udp-services-configmap=$(POD_NAMESPACE)/udp-services
        - --validating-webhook=:8443
        - --validating-webhook-certificate=/usr/local/certificates/cert
        - --validating-webhook-key=/usr/local/certificates/key
```

## Install Echo Server

```shell=
helm install echo-server ./k8s_helm
``` 

```shell=
kubectl patch configmap tcp-services -n kube-system --patch '{"data":{"8888":"default/echo-server:8888"}}'
```

## Add ports to NGINX Ingress Controller Deployment

```shell=
kubectl patch deployment ingress-nginx-controller -n kube-system --patch "$(cat k8s_helm/patch/nginx-ingress-controller-patch.yaml)"
```

## Add ports to NGINX Ingress Controller Service

```shell=
kubectl patch service ingress-nginx-controller -n kube-system --patch "$(cat k8s_helm/patch/nginx-ingress-svc-controller-patch.yaml)"
```

## Create tunnel (if you run service on localhost)

```shell=
minikube tunnel
```

## TEST

```
kubectl get service -n kube-system ingress-nginx-controller

nc <EXTERNAL_IP> 8888
```

## Clean up


```shell=
# remove echo server
helm uninstall echo-server

# remove nginx-ingress-controller
helm uninstall nginx-ingress-controller -n kube-system

# remove nginx repo
helm repo remove ingress-nginx
```

## Issue

```
Error: rendered manifests contain a resource that already exists. Unable to continue with install: xxx "yyy" in namespace "" exists and cannot be imported into the current release: invalid ownership metadata; label validation error: missing key "app.kubernetes.io/managed-by": must be set to "Helm"; annotation validation error: missing key "meta.helm.sh/release-name": must be set to "ingress-nginx"; annotation validation error: missing key "meta.helm.sh/release-namespace": must be set to "kube-system"

solve: 
kubectl delete xxx yyy

```

## Reference

https://stackoverflow.com/questions/61430311/exposing-multiple-tcp-udp-services-using-a-single-loadbalancer-on-k8s

