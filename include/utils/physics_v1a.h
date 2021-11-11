#pragma once

#include <bullet/btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>


enum shapes{BOX, SPHERE}; // shapes per il collision detection, se si vogliono altre shapes contenitrici occorre aggiungerle e gestirle

class Physics
{
    public:
        btDiscreteDynamicsWorld* dynamicsWorld; // istanza con tutti gli elementi per la simulazione fisica
        btAlignedObjectArray<btCollisionShape*> collisionShapes; // vettore con tutte le shapes delle collisioni nella scena
        btDefaultCollisionConfiguration* collisionConfiguration; // istanza della classe contenente i parametri di configurazione
        btCollisionDispatcher* dispatcher; // 
        btBroadphaseInterface* overlappingPairCache; // metodo per broadphase collision detection
        btSequentialImpulseConstraintSolver* solver; // constraint solver

        Physics()
        {
            this->collisionConfiguration = new btDefaultCollisionConfiguration();
            this->dispatcher = new btCollisionDispatcher(this->collisionConfiguration);
            this->overlappingPairCache = new btDbvtBroadphase();
            this->solver = new btSequentialImpulseConstraintSolver();

            this->dynamicsWorld = new btDiscreteDynamicsWorld(this->dispatcher, this->overlappingPairCache, this->solver, this->collisionConfiguration);

            this->dynamicsWorld->setGravity(btVector3(0.0f, -9.8f, 0.0f));
        }

        btRigidBody* createRigidBody(int type, glm::vec3 pos, glm::vec3 size, glm::vec3 rot, float m, float friction, float restitution) // type = box or sphere
        {
            btCollisionShape* cShape = NULL;

            btVector3 position = btVector3(pos.x, pos.y, pos.z); // manual convertion
            btQuaternion rotation; // i quaternioni per la simulazione fisica 
            rotation.setEuler(rot.x, rot.y, rot.z);

            if (type == BOX)
            {
                btVector3 dim = btVector3(size.x, size.y, size.z);
                cShape = new btBoxShape(dim);
            }
            else if (type == SPHERE)
            {
                cShape = new btSphereShape(size.x);
            }

            this->collisionShapes.push_back(cShape);

            btTransform objTransform;
            objTransform.setIdentity();
            objTransform.setRotation(rotation);
            objTransform.setOrigin(position);

            btScalar mass = m;
            bool isDynamic = (mass != 0.0f); // settiamo il rigid body come statico o come dinamico in base alla massa data (convenzione)

            btVector3 localInertia(0.0f, 0.0f, 0.0f);
            if (isDynamic)
                cShape->calculateLocalInertia(mass, localInertia); // solo se è dinamico ha senso calcolare l'inerzia
            
            btDefaultMotionState* motionState = new btDefaultMotionState(objTransform);

            btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, cShape, localInertia);
            rbInfo.m_friction = friction;
            rbInfo.m_restitution = restitution;

            btRigidBody* body = new btRigidBody(rbInfo);

            this->dynamicsWorld->addRigidBody(body);

            return body;
        }

        void Clear() // clean up manuale
        {
            for (int i = this->dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--)
            {
                btCollisionObject* obj = this->dynamicsWorld->getCollisionObjectArray()[i];
                btRigidBody* body = btRigidBody::upcast(obj); // per usare il metodo delete per i rigid body si parte da btCollisionObject, che è un'upper class di RigidBody
                if (body && body->getMotionState())
                    delete body->getMotionState();
                this->dynamicsWorld->removeCollisionObject(obj);
                delete obj;
            }

            delete this->dynamicsWorld;
            delete this->solver;
            delete this->overlappingPairCache;
            delete this->dispatcher;
            delete this->collisionConfiguration;
            
            this->collisionShapes.clear();
        }
};

